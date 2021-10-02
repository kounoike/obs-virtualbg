#include "plugin.hpp"
#include <chrono>
#include <dml_provider_factory.h>
#include <media-io/video-scaler.h>
#include <obs-module.h>
#include <obs.h>
#include <onnxruntime_cxx_api.h>

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
  float *feedback_buffer;
};

float lut[256];

const char *detector_get_name(void *data) {
  UNUSED_PARAMETER(data);
  return obs_module_text("VirtualBackGroundDetectorFilter");
}

void detector_destroy(void *data) {
  blog(LOG_INFO, "detector_destroy");
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
  if (filter_data->feedback_buffer) {
    bfree(filter_data->feedback_buffer);
  }

  bfree(filter_data);
}

void detector_update(void *data, obs_data_t *settings) {
  virtual_bg_filter_data *filter_data = static_cast<virtual_bg_filter_data *>(data);
  if (filter_data == NULL) {
    return;
  }
  UNUSED_PARAMETER(settings);
  blog(LOG_INFO, "detector_update");

  Ort::SessionOptions sessionOptions;

  sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
  sessionOptions.DisableMemPattern();
  sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
  Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0));

  char *modelPath = obs_module_file("model.onnx");
  size_t newSize = strlen(modelPath) + 1;
  wchar_t *wcharModelPath = static_cast<wchar_t *>(bzalloc((newSize) * sizeof(wchar_t)));
  size_t convertedChars = 0;

  mbstowcs_s(&convertedChars, wcharModelPath, newSize, modelPath, _TRUNCATE);
  bfree(modelPath);

  try {
    static Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "virtual_bg inference");
    filter_data->session.reset(new Ort::Session(env, wcharModelPath, sessionOptions));
  } catch (const std::exception &ex) {
    blog(LOG_ERROR, "Can't create Session error: %s", ex.what());
    bfree(wcharModelPath);
    return;
  }
  bfree(wcharModelPath);

  filter_data->input_names[0] = filter_data->session->GetInputName(0, *filter_data->allocator);
  filter_data->output_names[0] = filter_data->session->GetOutputName(0, *filter_data->allocator);
  auto input_info = filter_data->session->GetInputTypeInfo(0);
  auto output_info = filter_data->session->GetOutputTypeInfo(0);
  auto input_dims = input_info.GetTensorTypeAndShapeInfo().GetShape();
  auto output_dims = output_info.GetTensorTypeAndShapeInfo().GetShape();
  filter_data->tensor_width = input_dims[2];
  filter_data->tensor_height = input_dims[1];
  if (filter_data->input_u8_buffer) {
    bfree(filter_data->input_u8_buffer);
  }
  filter_data->input_u8_buffer =
      (uint8_t *)bmalloc(filter_data->tensor_width * filter_data->tensor_height * 3);
  blog(LOG_INFO, "model loaded input:%s tensor: %dx%d", filter_data->input_names[0],
       filter_data->tensor_width, filter_data->tensor_height);

  filter_data->input_tensor =
      Ort::Value::CreateTensor<float>(*filter_data->allocator, input_dims.data(), input_dims.size());
  filter_data->output_tensor =
      Ort::Value::CreateTensor<float>(*filter_data->allocator, output_dims.data(), output_dims.size());

  if (filter_data->preprocess_scaler) {
    video_scaler_destroy(filter_data->preprocess_scaler);
    filter_data->preprocess_scaler = NULL;
  }
  blog(LOG_INFO, "vitual_bg_update done.");
}

void *detector_create(obs_data_t *settings, obs_source_t *source) {
  blog(LOG_INFO, "Start detector_create");
  UNUSED_PARAMETER(settings);
  struct virtual_bg_filter_data *filter_data =
      reinterpret_cast<virtual_bg_filter_data *>(bzalloc(sizeof(struct virtual_bg_filter_data)));
  filter_data->self = source;
  try {
    std::string instance_name{"virtual-background-inference"};
    filter_data->allocator.reset(new Ort::AllocatorWithDefaultOptions());
  } catch (const std::exception &ex) {
    blog(LOG_ERROR, "create failed %s");
    return NULL;
  }
  blog(LOG_INFO, "ORT Session Created");

  blog(LOG_INFO, "detector_create");
  detector_update(filter_data, settings);
  if (filter_data->feedback_buffer) {
    bfree(filter_data->feedback_buffer);
  }
  filter_data->feedback_buffer =
      (float *)bzalloc(sizeof(float) * filter_data->tensor_width * filter_data->tensor_height);

  for (int i = 0; i < 256; ++i) {
    lut[i] = i / 255.0f;
  }

  return filter_data;
}

struct obs_source_frame *detector_filter_video(void *data, struct obs_source_frame *frame) {
  auto start = std::chrono::high_resolution_clock::now();
  virtual_bg_filter_data *filter_data = static_cast<virtual_bg_filter_data *>(data);
  if (filter_data == NULL) {
    return frame;
  }
  if (filter_data->parent == NULL) {
    filter_data->parent = obs_filter_get_parent(filter_data->self);
    create_mask_data(filter_data->parent, filter_data->tensor_width, filter_data->tensor_height);
    blog(LOG_INFO, "detector set parent: %X %s", filter_data->parent,
         obs_source_get_name(filter_data->parent));
  }

  if (filter_data->frame_width != frame->width || filter_data->frame_height != frame->height ||
      filter_data->frame_format != frame->format || filter_data->frame_full_range != frame->full_range) {
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
    blog(LOG_INFO, "frame: %dx%d tensor: %dx%d", frame->width, frame->height, filter_data->tensor_width,
         filter_data->tensor_height);
    struct video_scale_info frame_scaler_info {
      frame->format, frame->width, frame->height, frame->full_range ? VIDEO_RANGE_FULL : VIDEO_RANGE_DEFAULT,
          VIDEO_CS_DEFAULT
    };
    struct video_scale_info tensor_scaler_info {
      VIDEO_FORMAT_BGR3, (uint32_t)filter_data->tensor_width,
          (uint32_t)filter_data->tensor_height, VIDEO_RANGE_DEFAULT, VIDEO_CS_DEFAULT
    };
    int ret = video_scaler_create(&filter_data->preprocess_scaler, &tensor_scaler_info, &frame_scaler_info,
                                  VIDEO_SCALE_DEFAULT);
    if (ret != 0) {
      blog(LOG_ERROR, "Can't create video_scaler_create %d", ret);
      return frame;
    }
  }

  const uint32_t linesize[] = {(uint32_t)filter_data->tensor_width * 3};
  video_scaler_scale(filter_data->preprocess_scaler, &filter_data->input_u8_buffer, linesize, frame->data,
                     frame->linesize);
  float *tensor_buffer = filter_data->input_tensor.GetTensorMutableData<float>();
  for (int i = 0; i < filter_data->tensor_width * filter_data->tensor_height; ++i) {
    tensor_buffer[i * 3 + 0] = lut[filter_data->input_u8_buffer[i * 3 + 2]];
    tensor_buffer[i * 3 + 1] = lut[filter_data->input_u8_buffer[i * 3 + 1]];
    tensor_buffer[i * 3 + 2] = lut[filter_data->input_u8_buffer[i * 3 + 0]];
  }
  filter_data->session->Run(Ort::RunOptions(NULL), filter_data->input_names, &filter_data->input_tensor, 1,
                            filter_data->output_names, &filter_data->output_tensor, 1);

  uint8_t *buffer =
      (uint8_t *)bmalloc(sizeof(uint8_t) * filter_data->tensor_width * filter_data->tensor_height);
  const float *tensor_buffer2 = filter_data->output_tensor.GetTensorData<float>();
  for (int i = 0; i < filter_data->tensor_width * filter_data->tensor_height; ++i) {
    float val = tensor_buffer2[i] * 0.9f + filter_data->feedback_buffer[i] * 0.1f;
    // buffer[i] = val < 0.4f ? 0 : 255;
    buffer[i] = val * 255.0f;
    filter_data->feedback_buffer[i] = buffer[i] / 255.0f;
  }

  set_mask_data(filter_data->parent, buffer);
  bfree(buffer);

  if (filter_data->cnt % 300 == 0) {
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    blog(LOG_INFO, "called filter_video %d duration: %f", filter_data->cnt, duration.count() / 1000000.0);
  }
  filter_data->cnt++;
  return frame;
}

struct obs_source_info obs_virtualbg_detector_source_info {
  .id = "virtualbg", .type = OBS_SOURCE_TYPE_FILTER, .output_flags = OBS_SOURCE_ASYNC_VIDEO,
  .get_name = detector_get_name, .create = detector_create, .destroy = detector_destroy,
  .update = detector_update, .filter_video = detector_filter_video
};
