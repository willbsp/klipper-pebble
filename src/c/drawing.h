#pragma once

#include "messaging.h"
#include <pebble.h>

void drawing_draw_bed(GContext *ctx, GRect bounds, int anim_frame, int target_temp);

void drawing_draw_nozzle(GContext *ctx, GRect bounds, int anim_frame, int target_temp);

void drawing_draw_print(GContext *ctx, GRect bounds, int anim_frame, int progress,
                        PrintState state);
