#include <map>
#include <memory>
#include <mutex>
#include <obs-module.h>

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE("obs-virtualbg", "en-US")

struct mask_data {
  uint32_t width;
  uint32_t height;
  uint8_t *buffer;
};

std::map<obs_source_t *, mask_data> data_map;
std::mutex data_map_mtx;

void create_mask_data(obs_source_t *source, uint32_t width, uint32_t height) {
  blog(LOG_INFO, "[DEBUG] create_mask_data %dx%d", width, height);
  std::lock_guard<std::mutex> lock(data_map_mtx);
  if (data_map.find(source) != data_map.end()) {
    auto data = data_map.at(source);
    bfree(data.buffer);
    data_map.erase(source);
  }
  struct mask_data data {
    .width = width, .height = height, .buffer = (uint8_t *)bzalloc(sizeof(uint8_t) * width * height),
  };
  data_map.insert_or_assign(source, data);
}

void set_mask_data(obs_source_t *source, const uint8_t *buffer) {
  std::lock_guard<std::mutex> lock(data_map_mtx);
  try {
    auto data = data_map.at(source);
    memcpy(data.buffer, buffer, sizeof(uint8_t) * data.width * data.height);
  } catch (const std::out_of_range &ex) {
    return;
  }
}

void get_mask_data(obs_source_t *source, uint8_t *buffer) {
  std::lock_guard<std::mutex> lock(data_map_mtx);
  try {
    auto data = data_map.at(source);
    memcpy(buffer, data.buffer, sizeof(uint8_t) * data.width * data.height);
  } catch (const std::out_of_range &ex) {
    return;
  }
}

uint32_t get_mask_width(obs_source_t *source) {
  std::lock_guard<std::mutex> lock(data_map_mtx);
  try {
    return data_map.at(source).width;
  } catch (const std::out_of_range &ex) {
    return 0;
  }
}

uint32_t get_mask_height(obs_source_t *source) {
  std::lock_guard<std::mutex> lock(data_map_mtx);
  try {
    return data_map.at(source).height;
  } catch (const std::out_of_range &ex) {
    return 0;
  }
}

void delete_mask_data(obs_source_t *source) {
  std::lock_guard<std::mutex> lock(data_map_mtx);
  try {
    bfree(data_map.at(source).buffer);
    data_map.erase(source);
  } catch (const std::out_of_range &ex) {
    return;
  }
}

extern struct obs_source_info obs_virtualbg_detector_source_info;
extern struct obs_source_info obs_virtualbg_render_source_info;

bool obs_module_load(void) {
  obs_register_source(&obs_virtualbg_detector_source_info);
  obs_register_source(&obs_virtualbg_render_source_info);
  return true;
}

MODULE_EXPORT const char *obs_module_description(void) { return "Virtual background filter plugin"; }
