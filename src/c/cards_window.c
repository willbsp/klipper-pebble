#include "cards_window.h"

#include "drawing.h"
#include "messaging.h"
#include <pebble.h>
#include <stdio.h>

#define NUM_CARDS   3
#define CARD_BED    0
#define CARD_NOZZLE 1
#define CARD_PRINT  2

#define BG_BED    GColorOxfordBlue
#define BG_NOZZLE GColorBulgarianRose
#define BG_PRINT  GColorMidnightGreen

#define LABEL_TEXT_BED_TEMP "BED TEMP"
#define LABEL_TEXT_NOZZLE   "NOZZLE"
#define LABEL_TEXT_PRINT    "PRINT"

#define VALUE_TEXT_DONE  "Done"
#define VALUE_TEXT_ERROR "Error"
#define VALUE_TEXT_IDLE  "Idle"

#define SUBTEXT_TEXT_HEATING    "Heating.."
#define SUBTEXT_TEXT_AT_TARGET  "At target"
#define SUBTEXT_TEXT_HEATER_OFF "Heater off"
#define SUBTEXT_TEXT_COMPLETE   "Print complete"
#define SUBTEXT_TEXT_PAUSED     "Paused"
#define SUBTEXT_TEXT_ERROR      "Check printer"
#define SUBTEXT_TEXT_READY      "Ready"

#define ANIM_FRAME_MS 80

#define CARD_TRANSITION_MS          350
#define CARD_TRANSITION_WIPE_OFFSET 40

static Window *s_window;

static TextLayer *s_label_layer;
static TextLayer *s_value_layer;
static TextLayer *s_subtext_layer;
static StatusBarLayer *s_status_bar;
static Layer *s_canvas_layer;

static int s_current_card = CARD_NOZZLE;

static int s_transition_direction = 0;
static int s_transition_progress = 0;

static char s_label_buf[32];
static char s_value_buf[32];
static char s_subtext_buf[32];

static AppTimer *s_anim_timer = NULL;
static int s_anim_frame = 0;

static int s_icon_area_h = 0;

static void prv_draw_card_icon(GContext *ctx, int card, GRect bounds) {
  switch (card) {
  case CARD_BED:
    drawing_draw_bed(ctx, bounds, s_anim_frame, messaging_get_bed_target());
    break;
  case CARD_NOZZLE:
    drawing_draw_nozzle(ctx, bounds, s_anim_frame, messaging_get_nozzle_target());
    break;
  case CARD_PRINT:
    drawing_draw_print(ctx, bounds, s_anim_frame, messaging_get_print_progress(),
                       messaging_get_print_state());
    break;
  }
}

static GColor prv_bg_color_for_card(int card) {
  switch (card) {
  case CARD_BED:
    return BG_BED;
  case CARD_NOZZLE:
    return BG_NOZZLE;
  case CARD_PRINT:
    return BG_PRINT;
  default:
    return GColorBlack;
  }
}

static void prv_format_time_remaining(int seconds, char *buf, int buf_size) {
  if (seconds <= 0) {
    snprintf(buf, buf_size, "-- : --");
    return;
  }
  int h = seconds / 3600;
  int m = (seconds % 3600) / 60;
  if (h > 0) {
    snprintf(buf, buf_size, "%dh %02dm left", h, m);
  } else {
    snprintf(buf, buf_size, "%dm left", m);
  }
}

static void prv_set_heater_content(const char *label_text, int temp, int target) {
  snprintf(s_label_buf, sizeof(s_label_buf), "%s", label_text);
  snprintf(s_value_buf, sizeof(s_value_buf), "%d° / %d°", temp, target);
  if (target > 0 && temp < target) {
    snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_HEATING);
  } else if (target > 0) {
    snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_AT_TARGET);
  } else {
    snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_HEATER_OFF);
  }
}

static void prv_update_card_text(void) {
  const PrintState print_state = messaging_get_print_state();

  switch (s_current_card) {
  case CARD_BED:
    prv_set_heater_content(LABEL_TEXT_BED_TEMP, messaging_get_bed_temp(),
                           messaging_get_bed_target());
    break;
  case CARD_NOZZLE:
    prv_set_heater_content(LABEL_TEXT_NOZZLE, messaging_get_nozzle_temp(),
                           messaging_get_nozzle_target());
    break;
  case CARD_PRINT:
    snprintf(s_label_buf, sizeof(s_label_buf), LABEL_TEXT_PRINT);
    if (print_state == PRINT_PRINTING) {
      snprintf(s_value_buf, sizeof(s_value_buf), "%d%%", messaging_get_print_progress());
      prv_format_time_remaining(messaging_get_print_time_left(), s_subtext_buf,
                                sizeof(s_subtext_buf));
    } else if (print_state == PRINT_COMPLETE) {
      snprintf(s_value_buf, sizeof(s_value_buf), VALUE_TEXT_DONE);
      snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_COMPLETE);
    } else if (print_state == PRINT_PAUSED) {
      snprintf(s_value_buf, sizeof(s_value_buf), "%d%%", messaging_get_print_progress());
      snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_PAUSED);
    } else if (print_state == PRINT_ERROR) {
      snprintf(s_value_buf, sizeof(s_value_buf), VALUE_TEXT_ERROR);
      snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_ERROR);
    } else {
      snprintf(s_value_buf, sizeof(s_value_buf), VALUE_TEXT_IDLE);
      snprintf(s_subtext_buf, sizeof(s_subtext_buf), SUBTEXT_TEXT_READY);
    }
    break;
  }

  text_layer_set_text(s_label_layer, s_label_buf);
  text_layer_set_text(s_value_layer, s_value_buf);
  text_layer_set_text(s_subtext_layer, s_subtext_buf);
  layer_mark_dirty(s_canvas_layer);
}

static void prv_draw_transition(Layer *layer, GContext *context) {
  GRect bounds = layer_get_bounds(layer);
  int pct = (int)((s_transition_progress * 100) / ANIMATION_NORMALIZED_MAX);

  int total_travel = bounds.size.h + CARD_TRANSITION_WIPE_OFFSET;
  int wipe_y;

  if (s_transition_direction > 0) {
    wipe_y = bounds.size.h + CARD_TRANSITION_WIPE_OFFSET / 2 - (total_travel * pct / 100);
  } else {
    wipe_y = -CARD_TRANSITION_WIPE_OFFSET / 2 + (total_travel * pct / 100);
  }

  int y_left = wipe_y - CARD_TRANSITION_WIPE_OFFSET / 2;
  int y_right = wipe_y + CARD_TRANSITION_WIPE_OFFSET / 2;

  graphics_context_set_fill_color(context, prv_bg_color_for_card(s_current_card));
  graphics_fill_rect(context, bounds, 0, GCornerNone);
  graphics_context_set_fill_color(context,
                                  prv_bg_color_for_card(s_current_card + s_transition_direction));

  GPoint new_poly[4];
  if (s_transition_direction > 0) {
    new_poly[0] = GPoint(0, y_left);
    new_poly[1] = GPoint(bounds.size.w, y_right);
    new_poly[2] = GPoint(bounds.size.w, bounds.size.h);
    new_poly[3] = GPoint(0, bounds.size.h);
  } else {
    new_poly[0] = GPoint(0, 0);
    new_poly[1] = GPoint(bounds.size.w, 0);
    new_poly[2] = GPoint(bounds.size.w, y_right);
    new_poly[3] = GPoint(0, y_left);
  }

  GPathInfo path_info = {.num_points = 4, .points = new_poly};
  GPath *path = gpath_create(&path_info);
  gpath_draw_filled(context, path);
  gpath_destroy(path);

  int icon_travel = bounds.size.h / 3;
  int old_offset = s_transition_direction * (icon_travel * pct / 100);
  int new_offset =
      -s_transition_direction * icon_travel + s_transition_direction * (icon_travel * pct / 100);

  if (pct < 70) {
    GRect old_bounds = bounds;
    old_bounds.size.h = s_icon_area_h + old_offset * 2;
    prv_draw_card_icon(context, s_current_card, old_bounds);
  }

  if (pct > 30) {
    GRect new_bounds = bounds;
    new_bounds.size.h = s_icon_area_h + new_offset * 2;
    prv_draw_card_icon(context, s_current_card + s_transition_direction, new_bounds);
  }
}

static void prv_canvas_update_proc(Layer *layer, GContext *context) {
  GRect bounds = layer_get_bounds(layer);
  if (s_transition_direction == 0) {
    graphics_context_set_fill_color(context, prv_bg_color_for_card(s_current_card));
    graphics_fill_rect(context, bounds, 0, GCornerNone);
    bounds.size.h = s_icon_area_h;
    prv_draw_card_icon(context, s_current_card, bounds);
  } else {
    prv_draw_transition(layer, context);
  }
}

static void prv_transition_setup(Animation *anim) {
  s_transition_progress = 0;
}

static void prv_transition_update(Animation *anim, const AnimationProgress progress) {
  s_transition_progress = progress;
  layer_mark_dirty(s_canvas_layer);
}

static void prv_transition_teardown(Animation *anim) {
  s_current_card += s_transition_direction;
  s_transition_direction = 0;
  s_transition_progress = 0;
  prv_update_card_text();
  layer_mark_dirty(s_canvas_layer);
}

static const AnimationImplementation s_transition_impl = {
    .setup = prv_transition_setup,
    .update = prv_transition_update,
    .teardown = prv_transition_teardown,
};

static void prv_start_card_transition(int direction) {
  s_transition_direction = direction;
  Animation *anim = animation_create();
  animation_set_duration(anim, CARD_TRANSITION_MS);
  animation_set_curve(anim, AnimationCurveEaseInOut);
  animation_set_implementation(anim, &s_transition_impl);
  animation_schedule(anim);
}

static void prv_anim_timer_callback(void *context) {
  s_anim_frame++;
  layer_mark_dirty(s_canvas_layer);
  s_anim_timer = app_timer_register(ANIM_FRAME_MS, prv_anim_timer_callback, NULL);
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_card >= NUM_CARDS - 1 || s_transition_direction != 0) {
    return;
  }
  prv_start_card_transition(1);
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_card <= 0 || s_transition_direction != 0) {
    return;
  }
  prv_start_card_transition(-1);
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_bar, GColorBlack, GColorWhite);
  status_bar_layer_set_separator_mode(s_status_bar, StatusBarLayerSeparatorModeNone);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));

  int content_y = STATUS_BAR_LAYER_HEIGHT;
  int content_h = bounds.size.h - STATUS_BAR_LAYER_HEIGHT;

  s_canvas_layer = layer_create(GRect(0, content_y, bounds.size.w, content_h));
  layer_set_update_proc(s_canvas_layer, prv_canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  int icon_h = (content_h * 55) / 100;
  s_icon_area_h = icon_h;

  int label_y = content_y + icon_h;
  s_label_layer = text_layer_create(GRect(0, label_y, bounds.size.w, 22));
  text_layer_set_background_color(s_label_layer, GColorClear);
  text_layer_set_text_color(s_label_layer, GColorLightGray);
  text_layer_set_text_alignment(s_label_layer, GTextAlignmentCenter);
  text_layer_set_font(s_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_label_layer));

  int value_y = label_y + 20;
  s_value_layer = text_layer_create(GRect(0, value_y, bounds.size.w, 36));
  text_layer_set_background_color(s_value_layer, GColorClear);
  text_layer_set_text_color(s_value_layer, GColorWhite);
  text_layer_set_text_alignment(s_value_layer, GTextAlignmentCenter);
  text_layer_set_font(s_value_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_value_layer));

  int sub_y = value_y + 32;
  s_subtext_layer = text_layer_create(GRect(0, sub_y, bounds.size.w, 22));
  text_layer_set_background_color(s_subtext_layer, GColorClear);
  text_layer_set_text_color(s_subtext_layer, GColorLightGray);
  text_layer_set_text_alignment(s_subtext_layer, GTextAlignmentCenter);
  text_layer_set_font(s_subtext_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_subtext_layer));

  prv_update_card_text();
  s_anim_timer = app_timer_register(ANIM_FRAME_MS, prv_anim_timer_callback, NULL);
}

static void prv_window_unload(Window *window) {
  app_timer_cancel(s_anim_timer);
  s_anim_timer = NULL;

  text_layer_destroy(s_label_layer);
  text_layer_destroy(s_value_layer);
  text_layer_destroy(s_subtext_layer);
  layer_destroy(s_canvas_layer);
  status_bar_layer_destroy(s_status_bar);

  window_destroy(window);
  s_window = NULL;
}

void cards_window_update(void) {
  if (!s_window) {
    return;
  }

  prv_update_card_text();
}

void cards_window_init() {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = prv_window_load,
                                           .unload = prv_window_unload,
                                       });
  window_stack_push(s_window, true);
}
