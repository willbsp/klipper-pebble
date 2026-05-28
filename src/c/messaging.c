#include "messaging.h"

#include "message_keys.auto.h"
#include <pebble.h>
#include <stdio.h>
#include <string.h>

#define CAMERA_CHUNK_SIZE 800

static const uint32_t s_inbox_size = 2048;
static const uint32_t s_outbox_size = 64;

static int s_nozzle_temp = 0, s_nozzle_target = 0;
static int s_bed_temp = 0, s_bed_target = 0;
static int s_print_progress = 0, s_print_time_left = 0;
static char s_print_state[32] = "standby";

static uint8_t s_camera_pixels[CAMERA_W * CAMERA_H];
static bool s_camera_ready = false;
static int s_camera_chunks_received = 0;

static MessagingUpdateCallback s_on_update = NULL;

int messaging_get_nozzle_temp(void) { return s_nozzle_temp; }
int messaging_get_nozzle_target(void) { return s_nozzle_target; }
int messaging_get_bed_temp(void) { return s_bed_temp; }
int messaging_get_bed_target(void) { return s_bed_target; }
int messaging_get_print_progress(void) { return s_print_progress; }
int messaging_get_print_time_left(void) { return s_print_time_left; }
const char *messaging_get_print_state(void) { return s_print_state; }

bool messaging_is_camera_ready(void) { return s_camera_ready; }
const uint8_t *messaging_get_camera_pixels(void) { return s_camera_pixels; }

void messaging_request_camera(void) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_uint8(iter, MESSAGE_KEY_CameraRequest, 1);
    app_message_outbox_send();
  }
}

static void prv_inbox_received_callback(DictionaryIterator *iter, void *context) {
  Tuple *t;

  t = dict_find(iter, MESSAGE_KEY_NozzleTemp);
  if (t) s_nozzle_temp = t->value->int32;

  t = dict_find(iter, MESSAGE_KEY_NozzleTarget);
  if (t) s_nozzle_target = t->value->int32;

  t = dict_find(iter, MESSAGE_KEY_BedTemp);
  if (t) s_bed_temp = t->value->int32;

  t = dict_find(iter, MESSAGE_KEY_BedTarget);
  if (t) s_bed_target = t->value->int32;

  t = dict_find(iter, MESSAGE_KEY_PrintProgress);
  if (t) s_print_progress = t->value->int32;

  t = dict_find(iter, MESSAGE_KEY_PrintTimeLeft);
  if (t) s_print_time_left = t->value->int32;

  t = dict_find(iter, MESSAGE_KEY_PrintState);
  if (t) snprintf(s_print_state, sizeof(s_print_state), "%s", t->value->cstring);

  Tuple *chunk_idx_t = dict_find(iter, MESSAGE_KEY_CameraChunkIndex);
  Tuple *chunk_tot_t = dict_find(iter, MESSAGE_KEY_CameraChunkTotal);
  Tuple *chunk_data_t = dict_find(iter, MESSAGE_KEY_CameraChunkData);

  if (chunk_idx_t && chunk_tot_t && chunk_data_t) {
    int index = chunk_idx_t->value->int32;
    int total = chunk_tot_t->value->int32;
    int offset = index * CAMERA_CHUNK_SIZE;
    int len = chunk_data_t->length;

    if (index == 0) {
      s_camera_ready = false;
      s_camera_chunks_received = 0;
    }

    if (offset + len <= (int)sizeof(s_camera_pixels)) {
      memcpy(s_camera_pixels + offset, chunk_data_t->value->data, len);
      s_camera_chunks_received++;
    }

    if (s_camera_chunks_received >= total) {
      s_camera_ready = true;
      if (s_on_update) s_on_update();
    }
  } else {
    if (s_on_update) s_on_update();
  }
}

static void prv_inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
}

void messaging_init(MessagingUpdateCallback on_update) {
  s_on_update = on_update;
  app_message_register_inbox_received(prv_inbox_received_callback);
  app_message_register_inbox_dropped(prv_inbox_dropped_callback);
  app_message_open(s_inbox_size, s_outbox_size);
}
