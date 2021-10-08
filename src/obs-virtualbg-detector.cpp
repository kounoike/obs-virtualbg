#include "plugin.hpp"
#include <HalideBuffer.h>
#include <blur.h>
#include <chrono>
#include <media-io/video-scaler.h>
#include <obs-module.h>
#include <obs.h>

#ifdef _WIN32
#include <dml_provider_factory.h>
#endif

#ifdef __APPLE__
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
// #include <onnxruntime/core/providers/cpu/cpu_provider_factory.h>
#else
#include <onnxruntime_cxx_api.h>
#endif

const char *USE_THRESHOLD = "UseThreashold";
const char *THRESHOLD_VALUE = "ThresholdValue";
const char *USE_MASK_BLUR = "UseMaskBlur";
const char *USE_GPU = "UseGpu";

struct virtual_bg_filter_data {
  obs_source_t *self;
  obs_source_t *parent;
  int cnt;
  std::unique_ptr<Ort::Session> session;
  std::unique_ptr<Ort::AllocatorWithDefaultOptions> allocator;
  video_scaler_t *preprocess_scaler;
  char *input_names[1];
  char *output_names[1];
  int64_t tensor_width;
  int64_t tensor_height;
  uint32_t frame_width;
  uint32_t frame_height;
  video_format frame_format;
  bool frame_full_range;
  Ort::Value input_tensor;
  Ort::Value output_tensor;
  uint8_t *input_u8_buffer;
  uint8_t *mask_u8_buffer;
  uint8_t *mask_blurred_u8_buffer;
  float *feedback_buffer;

  // settings
  bool use_threshold;
  double threshold;
  bool use_mask_blur;
  bool use_gpu;
};

float lut[256];

const char *detector_get_name(void *data) {
  UNUSED_PARAMETER(data);
  return obs_module_text("VirtualBackGroundDetectorFilter");
}

void detector_destroy(void *data) {
  virtual_bg_filter_data *filter_data = static_cast<virtual_bg_filter_data *>(data);
  if (filter_data == NULL) {
    return;
  }
  delete_mask_data(filter_data->parent);
  filter_data->allocator->Free(filter_data->input_names[0]);
  filter_data->allocator->Free(filter_data->output_names[0]);
  if (filter_data->preprocess_scaler) {
    video_scaler_destroy(filter_data->preprocess_scaler);
  }
  if (filter_data->input_u8_buffer) {
    bfree(filter_data->input_u8_buffer);
  }
  if (filter_data->mask_u8_buffer) {
    bfree(filter_data->mask_u8_buffer);
  }
  if (filter_data->mask_blurred_u8_buffer) {
    bfree(filter_data->mask_blurred_u8_buffer);
  }
  if (filter_data->feedback_buffer) {
    bfree(filter_data->feedback_buffer);
  }

  bfree(filter_data);
}

void detector_setup_ort_session_gpu(Ort::SessionOptions &sessionOptions) {
#if _WIN32
  try {
    sessionOptions.DisableMemPattern();
    sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
    Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0));

  } catch (const std::exception &ex) {
    blog(LOG_ERROR,
         "[Virtual BG detector] Can't Append Execution Provider DML. Will use CPU inference (its so heavy). "
         "error: %s",
         ex.what());
  }
#endif
}

void detector_setup_ort_session_load_model(virtual_bg_filter_data *filter_data,
                                           Ort::SessionOptions &sessionOptions) {
  char *modelPath = obs_module_file("model.onnx");
  static Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "virtual_bg inference");

#if _WIN32
  size_t newSize = strlen(modelPath) + 1;
  wchar_t *wcharModelPath = static_cast<wchar_t *>(bzalloc((newSize) * sizeof(wchar_t)));
  size_t convertedChars = 0;

  mbstowcs_s(&convertedChars, wcharModelPath, newSize, modelPath, _TRUNCATE);
  bfree(modelPath);

  try {
    filter_data->session.reset(new Ort::Session(env, wcharModelPath, sessionOptions));
    blog(LOG_INFO, "[Virtual BG detector] Create OnnxRuntime session");
  } catch (const std::exception &ex) {
    blog(LOG_ERROR, "[Virtual BG detector] Can't create Session error: %s", ex.what());
    bfree(wcharModelPath);
    return;
  }
  bfree(wcharModelPath);
#else
  try {
    filter_data->session.reset(new Ort::Session(env, modelPath, sessionOptions));
  } catch (const std::exception &ex) {
    blog(LOG_ERROR, "[Virtual BG detector] Can't create session %s", ex.what());
    bfree(modelPath);
    return;
  }
  bfree(modelPath);
#endif
}

void detector_setup_ort_session(virtual_bg_filter_data *filter_data) {
  Ort::SessionOptions sessionOptions;

  sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

  if (filter_data->use_gpu) {
    detector_setup_ort_session_gpu(sessionOptions);
  }
  detector_setup_ort_session_load_model(filter_data, sessionOptions);
}

void detector_gather_session_information(virtual_bg_filter_data *filter_data) {
  if (filter_data->session.get() != nullptr) {
    try {
      if (filter_data->input_u8_buffer) {
        bfree(filter_data->input_u8_buffer);
        filter_data->input_u8_buffer = NULL;
      }
      if (filter_data->mask_u8_buffer) {
        bfree(filter_data->mask_u8_buffer);
        filter_data->mask_u8_buffer = NULL;
      }
      if (filter_data->mask_blurred_u8_buffer) {
        bfree(filter_data->mask_blurred_u8_buffer);
        filter_data->mask_blurred_u8_buffer = NULL;
      }
      if (filter_data->feedback_buffer) {
        bfree(filter_data->feedback_buffer);
        filter_data->feedback_buffer = NULL;
      }
      filter_data->input_names[0] = filter_data->session->GetInputName(0, *filter_data->allocator);
      filter_data->output_names[0] = filter_data->session->GetOutputName(0, *filter_data->allocator);
      auto input_info = filter_data->session->GetInputTypeInfo(0);
      auto output_info = filter_data->session->GetOutputTypeInfo(0);
      auto input_dims = input_info.GetTensorTypeAndShapeInfo().GetShape();
      auto output_dims = output_info.GetTensorTypeAndShapeInfo().GetShape();
      filter_data->tensor_width = input_dims[2];
      filter_data->tensor_height = input_dims[1];
      filter_data->input_u8_buffer =
          (uint8_t *)bmalloc(sizeof(uint8_t) * filter_data->tensor_width * filter_data->tensor_height * 3);
      filter_data->mask_u8_buffer =
          (uint8_t *)bmalloc(sizeof(uint8_t) * filter_data->tensor_width * filter_data->tensor_height);
      filter_data->mask_blurred_u8_buffer =
          (uint8_t *)bmalloc(sizeof(uint8_t) * filter_data->tensor_width * filter_data->tensor_height);
      filter_data->feedback_buffer =
          (float *)bzalloc(sizeof(float) * filter_data->tensor_width * filter_data->tensor_height);

      filter_data->input_tensor =
          Ort::Value::CreateTensor<float>(*filter_data->allocator, input_dims.data(), input_dims.size());
      filter_data->output_tensor =
          Ort::Value::CreateTensor<float>(*filter_data->allocator, output_dims.data(), output_dims.size());
    } catch (const std::exception &ex) {
      blog(LOG_ERROR, "[Virtual BG detector] somethins happens during gathering model information. error: %s",
           ex.what());
    }
  }
}

void detector_update(void *data, obs_data_t *settings) {
  virtual_bg_filter_data *filter_data = static_cast<virtual_bg_filter_data *>(data);
  if (filter_data == NULL) {
    return;
  }

  filter_data->use_threshold = obs_data_get_bool(settings, USE_THRESHOLD);
  filter_data->threshold = (float)obs_data_get_double(settings, THRESHOLD_VALUE);
  filter_data->use_mask_blur = obs_data_get_bool(settings, USE_MASK_BLUR);

#ifdef _WIN32
  filter_data->use_gpu = obs_data_get_bool(settings, USE_GPU);
#endif

  if (filter_data->preprocess_scaler) {
    video_scaler_destroy(filter_data->preprocess_scaler);
    filter_data->preprocess_scaler = NULL;
  }

  detector_setup_ort_session(filter_data);
  detector_gather_session_information(filter_data);
}

void detector_setup_lut() {
  for (int i = 0; i < 256; ++i) {
    lut[i] = i / 255.0f;
  }
}

void *detector_create(obs_data_t *settings, obs_source_t *source) {
  UNUSED_PARAMETER(settings);
  struct virtual_bg_filter_data *filter_data =
      reinterpret_cast<virtual_bg_filter_data *>(bzalloc(sizeof(struct virtual_bg_filter_data)));
  filter_data->self = source;
  try {
    std::string instance_name{"virtual-background-inference"};
    filter_data->allocator.reset(new Ort::AllocatorWithDefaultOptions());
  } catch (const std::exception &ex) {
    blog(LOG_ERROR, "[Virtual BG detector] detector_create failed %s", ex.what());
    return NULL;
  }

  detector_update(filter_data, settings);
  detector_setup_lut();

  return filter_data;
}

void detector_defaults(obs_data_t *settings) {
  obs_data_set_default_bool(settings, USE_THRESHOLD, true);
  obs_data_set_default_double(settings, THRESHOLD_VALUE, 0.5);
  obs_data_set_default_bool(settings, USE_MASK_BLUR, true);
#if _WIN32
  obs_data_set_default_bool(settings, USE_GPU, true);
#endif
}

obs_properties_t *detector_properties(void *data) {
  UNUSED_PARAMETER(data);
  obs_properties_t *ppts = obs_properties_create();
  obs_properties_add_bool(ppts, USE_THRESHOLD, obs_module_text(USE_THRESHOLD));
  obs_properties_add_float_slider(ppts, THRESHOLD_VALUE, obs_module_text(THRESHOLD_VALUE), 0.0, 1.0, 0.05);
  obs_properties_add_bool(ppts, USE_MASK_BLUR, obs_module_text(USE_MASK_BLUR));
#if _WIN32
  obs_properties_add_bool(ppts, USE_GPU, obs_module_text(USE_GPU));
#endif

  return ppts;
}

void detector_setup_preprocess_scaler(virtual_bg_filter_data *filter_data, struct obs_source_frame *frame) {
  struct video_scale_info frame_scaler_info {
    frame->format, frame->width, frame->height, frame->full_range ? VIDEO_RANGE_FULL : VIDEO_RANGE_DEFAULT,
        VIDEO_CS_DEFAULT
  };
  struct video_scale_info tensor_scaler_info {
    VIDEO_FORMAT_BGR3, (uint32_t)filter_data->tensor_width,
        (uint32_t)filter_data->tensor_height, VIDEO_RANGE_DEFAULT, VIDEO_CS_DEFAULT
  };
  int ret = video_scaler_create(&filter_data->preprocess_scaler, &tensor_scaler_info, &frame_scaler_info,
                                VIDEO_SCALE_BICUBIC);
  if (ret != 0) {
    blog(LOG_ERROR, "[Virtual BG detector] Can't create video_scaler_create %d", ret);
    throw new std::runtime_error("Cant create video_scaler_create");
  } else {
    blog(LOG_INFO, "[Virtual BG detector] video_scaler_create success. %dx%d -> %dx%d",
         filter_data->frame_width, filter_data->frame_height, filter_data->tensor_width,
         filter_data->tensor_height);
  }
}

void detector_preprocess(virtual_bg_filter_data *filter_data, struct obs_source_frame *frame) {
  const uint32_t linesize[] = {(uint32_t)filter_data->tensor_width * 3};
  if (!video_scaler_scale(filter_data->preprocess_scaler, &filter_data->input_u8_buffer, linesize,
                          frame->data, frame->linesize)) {
    blog(LOG_ERROR, "[Virtual BG detector] video_scaler_scale failed.");
    throw new std::runtime_error("video_scaler_scale failed.");
  }

  float *tensor_buffer = filter_data->input_tensor.GetTensorMutableData<float>();
  for (int i = 0; i < filter_data->tensor_width * filter_data->tensor_height; ++i) {
    tensor_buffer[i * 3 + 0] = lut[filter_data->input_u8_buffer[i * 3 + 2]];
    tensor_buffer[i * 3 + 1] = lut[filter_data->input_u8_buffer[i * 3 + 1]];
    tensor_buffer[i * 3 + 2] = lut[filter_data->input_u8_buffer[i * 3 + 0]];
  }
}

void detector_inference(virtual_bg_filter_data *filter_data) {
  try {
    filter_data->session->Run(Ort::RunOptions(NULL), filter_data->input_names, &filter_data->input_tensor, 1,
                              filter_data->output_names, &filter_data->output_tensor, 1);
  } catch (const std::exception &ex) {
    blog(LOG_ERROR, "[Virtual BG detector] Error at detector_inference. error: %s", ex.what());
    throw ex;
  }
}

void detector_postprocess(virtual_bg_filter_data *filter_data) {
  const float *tensor_buffer2 = filter_data->output_tensor.GetTensorData<float>();
  if (filter_data->use_threshold) {
    for (int i = 0; i < filter_data->tensor_width * filter_data->tensor_height; ++i) {
      float val = tensor_buffer2[i] * 0.9f + filter_data->feedback_buffer[i] * 0.1f;
      filter_data->mask_u8_buffer[i] = val >= filter_data->threshold ? 255 : 0;
    }
  } else {
    for (int i = 0; i < filter_data->tensor_width * filter_data->tensor_height; ++i) {
      float val = tensor_buffer2[i] * 0.8f + filter_data->feedback_buffer[i] * 0.2f;
      filter_data->mask_u8_buffer[i] = val * 255.0f;
    }
  }

  if (filter_data->use_mask_blur) {
    Halide::Runtime::Buffer<uint8_t> input{filter_data->mask_u8_buffer, (int)filter_data->tensor_width,
                                           filter_data->tensor_height};
    Halide::Runtime::Buffer<uint8_t> output{filter_data->mask_blurred_u8_buffer,
                                            (int)filter_data->tensor_width, filter_data->tensor_height};

    blur(input, output);
    for (int i = 0; i < filter_data->tensor_width * filter_data->tensor_height; ++i) {
      filter_data->feedback_buffer[i] = filter_data->mask_blurred_u8_buffer[i] / 255.0f;
    }

    set_mask_data(filter_data->parent, filter_data->mask_blurred_u8_buffer);
  } else {
    set_mask_data(filter_data->parent, filter_data->mask_u8_buffer);
  }
}
struct obs_source_frame *detector_filter_video(void *data, struct obs_source_frame *frame) {
  try {
    auto start = std::chrono::high_resolution_clock::now();
    virtual_bg_filter_data *filter_data = static_cast<virtual_bg_filter_data *>(data);
    if (filter_data == NULL || frame == NULL || frame->width == 0 || frame->height == 0) {
      return frame;
    }
    if (filter_data->parent == NULL) {
      filter_data->parent = obs_filter_get_parent(filter_data->self);
      if (filter_data->parent == NULL) {
        blog(LOG_ERROR, "[Virtual BG detector] Can't get obs_filter_get_parent");
        return frame;
      }
      create_mask_data(filter_data->parent, filter_data->tensor_width, filter_data->tensor_height);
    }

    if (filter_data->frame_width != frame->width || filter_data->frame_height != frame->height ||
        filter_data->frame_format != frame->format || filter_data->frame_full_range != frame->full_range) {
      blog(LOG_INFO, "[Virtual BG detector] frame size or format change detected");
      if (filter_data->preprocess_scaler) {
        video_scaler_destroy(filter_data->preprocess_scaler);
        filter_data->preprocess_scaler = NULL;
      }
      filter_data->frame_width = frame->width;
      filter_data->frame_height = frame->height;
      filter_data->frame_format = frame->format;
      filter_data->frame_full_range = frame->full_range;
    }

    if (!filter_data->preprocess_scaler) {
      detector_setup_preprocess_scaler(filter_data, frame);
    }
    detector_preprocess(filter_data, frame);
    detector_inference(filter_data);
    detector_postprocess(filter_data);

    if (filter_data->cnt % 300 == 0) {
      auto stop = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
      blog(LOG_INFO, "[Virtual BG detector] called filter_video %d duration: %f", filter_data->cnt,
           duration.count() / 1000000.0);
    }
    filter_data->cnt++;
  } catch (const std::exception &ex) {
    blog(LOG_ERROR, "[Virtual BG detector] error %s", ex.what());
  }
  return frame;
}

struct obs_source_info obs_virtualbg_detector_source_info {
  .id = "virtualbg", .type = OBS_SOURCE_TYPE_FILTER, .output_flags = OBS_SOURCE_ASYNC_VIDEO,
  .get_name = detector_get_name, .create = detector_create, .destroy = detector_destroy,
  .get_defaults = detector_defaults, .get_properties = detector_properties, .update = detector_update,
  .filter_video = detector_filter_video
};
