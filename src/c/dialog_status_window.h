#pragma once

#include <pebble.h>

void dialog_config_window_show(const char *title, const char *message, uint32_t resource_id);
void dialog_config_window_hide(void);
