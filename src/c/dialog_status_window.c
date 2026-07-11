#include "dialog_status_window.h"

#include <stdint.h>

static Window *s_window;
static TextLayer *s_body_layer, *s_title_layer;

static Layer *s_icon_layer;
static GDrawCommandImage *s_icon_pdc;

static const char *s_title;
static const char *s_message;
static uint32_t s_resource_id;

static void prv_icon_update_proc(Layer *layer, GContext *ctx) {
  if (!s_icon_pdc) {
    return;
  }
  // Draw at the layer origin; the layer is already positioned/sized to fit
  gdraw_command_image_draw(ctx, s_icon_pdc, GPointZero);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_icon_pdc = gdraw_command_image_create_with_resource(s_resource_id);

  // PDC equivalent of gbitmap_get_bounds(): ask the image for its size
  GSize icon_size = gdraw_command_image_get_bounds_size(s_icon_pdc);

  const int icon_v_center = (bounds.size.h - icon_size.h) / 2;
  const int icon_raise = 20;
  const GEdgeInsets icon_insets = GEdgeInsets(
      icon_v_center - icon_raise, (bounds.size.w - icon_size.w) / 2, icon_v_center + icon_raise);

  s_icon_layer = layer_create(grect_inset(bounds, icon_insets));
  layer_set_update_proc(s_icon_layer, prv_icon_update_proc);
  layer_add_child(window_layer, s_icon_layer);

  const GEdgeInsets title_insets = {.top = 10};
  s_title_layer = text_layer_create(grect_inset(bounds, title_insets));
  text_layer_set_text(s_title_layer, s_title);
  text_layer_set_text_color(s_title_layer, GColorBlack);
  text_layer_set_background_color(s_title_layer, GColorClear);
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_title_layer));

  const GEdgeInsets body_insets = {.top = 145, .right = 5, .left = 5};
  s_body_layer = text_layer_create(grect_inset(bounds, body_insets));
  text_layer_set_text(s_body_layer, s_message);
  text_layer_set_text_color(s_body_layer, GColorBlack);
  text_layer_set_background_color(s_body_layer, GColorClear);
  text_layer_set_font(s_body_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_body_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_body_layer));
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_body_layer);

  layer_destroy(s_icon_layer);
  gdraw_command_image_destroy(s_icon_pdc);
  s_icon_pdc = NULL;

  window_destroy(window);
  s_window = NULL;
}

static void prv_back_click_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop_all(true);
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, prv_back_click_handler);
}

void dialog_config_window_show(const char *title, const char *message, uint32_t resource_id) {
  s_title = title;
  s_message = message;
  s_resource_id = resource_id;
  if (!s_window) {
    s_window = window_create();
    window_set_background_color(s_window, GColorWhite);
    window_set_window_handlers(
        s_window, (WindowHandlers){.load = prv_window_load, .unload = prv_window_unload});
    window_set_click_config_provider(s_window, prv_click_config_provider);
  }
  if (!window_stack_contains_window(s_window)) {
    window_stack_push(s_window, true);
  }
}

void dialog_config_window_hide() {
  if (s_window) {
    window_stack_remove(s_window, true);
  }
}
