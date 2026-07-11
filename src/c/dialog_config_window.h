#pragma once

#include <pebble.h>

#define DIALOG_CONFIG_WINDOW_APP_NAME "Klipper Monitor"
#define DIALOG_CONFIG_WINDOW_MESSAGE  "Set up printer\n connection in the\nPebble app"

void dialog_config_window_show(void);
void dialog_config_window_hide(void);
