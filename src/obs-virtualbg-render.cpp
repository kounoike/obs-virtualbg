#include "plugin.hpp"
#include <exception>
#include <obs-module.h>
#include <obs.h>

struct render_filter_data {
  obs_source_t *self;
  obs_source_t *parent;
  gs_effect_t *effect;
  gs_texture_t *texture;
  gs_eparam_t *mask_param;
  uint8_t *mask_buffer;
  uint32_t mask_width;
  uint32_t mask_height;
  int cnt;
};

const char *render_get_name(void *data) {
  UNUSED_PARAMETER(data);
  return obs_module_text("VirtualBackGroundRenderFilter");
}

void render_destroy(void *data) {
  render_filter_data *filter_data = static_cast<render_filter_data *>(data);
  if (filter_data) {
    obs_enter_graphics();
    gs_effect_destroy(filter_data->effect);
    bfree(filter_data);
    obs_leave_graphics();
  }
  if (filter_data->mask_buffer) {
    bfree(filter_data->mask_buffer);
  }
  blog(LOG_INFO, "render_destroy");
}

void set_texture(render_filter_data *filter_data) {
  int width = get_mask_width(filter_data->parent);
  int height = get_mask_height(filter_data->parent);
  if (width < 0 || height < 0) {
    return;
  }

  obs_enter_graphics();
  if (filter_data->texture) {
    gs_texture_destroy(filter_data->texture);
  }
  filter_data->texture = gs_texture_create(width, height, GS_A8, 1, NULL, GS_DYNAMIC);
  obs_leave_graphics();
}

void render_update(void *data, obs_data_t *settings) {
  UNUSED_PARAMETER(settings);
  render_filter_data *filter_data = static_cast<render_filter_data *>(data);
}

void *render_create(obs_data_t *settings, obs_source_t *source) {
  UNUSED_PARAMETER(settings);

  obs_enter_graphics();

  struct render_filter_data *filter_data =
      reinterpret_cast<render_filter_data *>(bzalloc(sizeof(struct render_filter_data)));
  filter_data->self = source;
  auto effect_file = obs_module_file("virtualbg.effect");
  blog(LOG_INFO, "effect_file: %s", effect_file);
  filter_data->effect = gs_effect_create_from_file(effect_file, NULL);
  bfree(effect_file);
  if (!filter_data->effect) {
    render_destroy(filter_data);
    filter_data = NULL;
  }
  filter_data->mask_param = gs_effect_get_param_by_name(filter_data->effect, "mask");
  obs_leave_graphics();

  blog(LOG_INFO, "render_render_create");
  render_update(filter_data, settings);

  return filter_data;
}

void render_video_render(void *data, gs_effect_t *effect) {
  try {
    render_filter_data *filter_data = static_cast<render_filter_data *>(data);
    if (filter_data == NULL) {
      return;
    }
    if (filter_data->parent == NULL) {
      filter_data->parent = obs_filter_get_parent(filter_data->self);
      blog(LOG_INFO, "render set parent: %X %s", filter_data->parent,
           obs_source_get_name(filter_data->parent));
    }
    if (filter_data->texture == NULL) {
      set_texture(filter_data);
    }
    uint32_t width = (uint32_t)get_mask_width(filter_data->parent);
    uint32_t height = (uint32_t)get_mask_height(filter_data->parent);
    if (width == 0 || height == 0 || !filter_data->texture) {
      set_texture(filter_data);
      width = (uint32_t)get_mask_width(filter_data->parent);
      height = (uint32_t)get_mask_height(filter_data->parent);
      if (width == 0 || height == 0 || !filter_data->texture) {
        return;
      }
    }

    if (filter_data->mask_width != width || filter_data->mask_height != height) {
      if (filter_data->mask_buffer) {
        bfree(filter_data->mask_buffer);
        filter_data->mask_buffer = NULL;
      }
      filter_data->mask_width = width;
      filter_data->mask_height = height;
    }

    if (!filter_data->mask_buffer) {
      filter_data->mask_buffer = (uint8_t *)bmalloc(sizeof(uint8_t) * width * height);
    }
    get_mask_data(filter_data->parent, filter_data->mask_buffer);

    obs_enter_graphics();
    gs_texture_set_image(filter_data->texture, filter_data->mask_buffer, width, false);
    obs_leave_graphics();

    if (!obs_source_process_filter_begin(filter_data->self, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
      return;
    }
    gs_effect_set_texture(filter_data->mask_param, filter_data->texture);
    obs_source_process_filter_end(filter_data->self, filter_data->effect, 0, 0);

    if (filter_data->cnt % 300 == 0) {
      blog(LOG_INFO, "called %d", filter_data->cnt);
    }
    filter_data->cnt++;
  } catch (const std::exception &ex) {
    blog(LOG_ERROR, "error %s", ex.what());
  }
}

struct obs_source_info obs_virtualbg_render_source_info {
  .id = "virtualbg-render", .type = OBS_SOURCE_TYPE_FILTER,
  .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB, .get_name = render_get_name, .create = render_create,
  .destroy = render_destroy, .update = render_update, .video_render = render_video_render
};
