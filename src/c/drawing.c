#include "messaging.h"
#include <pebble.h>

void drawing_draw_bed(GContext *ctx, GRect bounds, int anim_frame, int target_temp) {
  int cx = bounds.size.w / 2;
  int cy = bounds.size.h / 2;

  // Bed plate
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_rect(ctx, GRect(cx - 40, cy + 8, 80, 10), 3, GCornersAll);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(cx - 38, cy + 8, 76, 3), 2, GCornersTop);

  // Legs
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, GRect(cx - 34, cy + 18, 6, 8), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(cx + 28, cy + 18, 6, 8), 0, GCornerNone);

  // Heat waves
  if (target_temp > 0) {
    graphics_context_set_stroke_color(ctx, GColorRed);
  } else {
    graphics_context_set_stroke_color(ctx, GColorDarkGray);
  }
  graphics_context_set_stroke_width(ctx, 2);

  int offsets[3] = {-20, 0, 20};
  int i;
  for (i = 0; i < 3; i++) {
    int wx = cx + offsets[i];
    int phase = (anim_frame + i * 3) % 12;
    int base_y = cy + 4;
    int y0 = base_y - phase * 2;
    int y1 = y0 - 6;
    int y2 = y1 - 6;
    if (y0 < cy + 6) {
      graphics_draw_line(ctx, GPoint(wx, y0), GPoint(wx - 4, y1));
      if (y1 > cy - 20) {
        graphics_draw_line(ctx, GPoint(wx - 4, y1), GPoint(wx + 4, y2));
      }
    }
  }
}

void drawing_draw_nozzle(GContext *ctx, GRect bounds, int anim_frame, int target_temp) {

  int cx = bounds.size.w / 2;
  int cy = bounds.size.h / 2;

  // Heatsink fins
  graphics_context_set_fill_color(ctx, GColorLightGray);
  int fin;
  for (fin = 0; fin < 4; fin++) {
    int fy = cy - 24 + fin * 7;
    graphics_fill_rect(ctx, GRect(cx - 20, fy, 40, 4), 1, GCornersAll);
  }

  // Heater block
  GColor block_color = (target_temp > 0) ? GColorChromeYellow : GColorDarkGray;
  graphics_context_set_fill_color(ctx, block_color);
  graphics_fill_rect(ctx, GRect(cx - 16, cy + 4, 32, 16), 3, GCornersAll);

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, "HE", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(cx - 16, cy + 2, 32, 16), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Nozzle tip
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_rect(ctx, GRect(cx - 5, cy + 20, 10, 6), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(cx - 2, cy + 26, 4, 4), 0, GCornerNone);

  // Filament drip
  if (target_temp > 0) {
    int drip_phase = anim_frame % 10;
    int drip_y = cy + 30 + drip_phase;
    int drip_r = (drip_phase < 5) ? 2 : 1;
    graphics_context_set_fill_color(ctx, GColorOrange);
    graphics_fill_circle(ctx, GPoint(cx, drip_y), drip_r);
  }

  // Pulsing heat glow
  if (target_temp > 0) {
    int pulse = anim_frame % 6;
    if (pulse < 3) {
      graphics_context_set_stroke_color(ctx, GColorRed);
      graphics_context_set_stroke_width(ctx, 1);
      graphics_draw_round_rect(
          ctx, GRect(cx - 18 - pulse, cy + 2 - pulse, 36 + pulse * 2, 20 + pulse * 2), 4);
    }
  }
}

void drawing_draw_print(GContext *ctx, GRect bounds, int anim_frame, int progress,
                        PrintState state) {

  int cx = bounds.size.w / 2;
  int cy = bounds.size.h / 2;

  // Printer frame
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_round_rect(ctx, GRect(cx - 36, cy - 18, 72, 52), 4);

  // Build plate
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, GRect(cx - 28, cy + 22, 56, 6), 1, GCornersAll);

  // Printed object
  int max_h = 30;
  int obj_h = (max_h * progress) / 100;
  if (obj_h < 2 && state == PRINT_PRINTING) {
    obj_h = 2;
  }
  if (obj_h > 0) {
    graphics_context_set_fill_color(ctx, GColorIslamicGreen);
    graphics_fill_rect(ctx, GRect(cx - 10, cy + 22 - obj_h, 20, obj_h), 0, GCornerNone);
  }

  // Moving nozzle
  if (state == PRINT_PRINTING) {
    int sweep = 30;
    int phase = anim_frame % 20;
    int nozzle_x;
    if (phase < 10) {
      nozzle_x = cx - sweep / 2 + (sweep * phase) / 10;
    } else {
      nozzle_x = cx + sweep / 2 - (sweep * (phase - 10)) / 10;
    }
    int nozzle_y = cy + 22 - obj_h - 5;
    if (nozzle_y < cy - 14) {
      nozzle_y = cy - 14;
    }

    graphics_context_set_fill_color(ctx, GColorChromeYellow);
    graphics_fill_rect(ctx, GRect(nozzle_x - 4, nozzle_y, 8, 5), 1, GCornersAll);
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, GRect(nozzle_x - 1, nozzle_y + 5, 2, 2), 0, GCornerNone);
  }

  // Status dot
  GColor status_color;
  if (state == PRINT_PRINTING) {
    status_color = (anim_frame % 8 < 5) ? GColorGreen : GColorIslamicGreen;
  } else if (state == PRINT_COMPLETE) {
    status_color = GColorGreen;
  } else if (state == PRINT_ERROR) {
    status_color = GColorRed;
  } else if (state == PRINT_PAUSED) {
    status_color = GColorChromeYellow;
  } else {
    status_color = GColorDarkGray;
  }
  graphics_context_set_fill_color(ctx, status_color);
  graphics_fill_circle(ctx, GPoint(cx + 28, cy - 12), 4);
}
