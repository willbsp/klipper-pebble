#pragma once

#include <pebble.h>

#define CAMERA_W 120
#define CAMERA_H 120

typedef void (*MessagingUpdateCallback)(void);

void messaging_init(MessagingUpdateCallback on_update);

int messaging_get_nozzle_temp(void);
int messaging_get_nozzle_target(void);
int messaging_get_bed_temp(void);
int messaging_get_bed_target(void);
int messaging_get_print_progress(void);
int messaging_get_print_time_left(void);
const char *messaging_get_print_state(void);

void messaging_request_camera(void);
bool messaging_is_camera_ready(void);
const uint8_t *messaging_get_camera_pixels(void);
