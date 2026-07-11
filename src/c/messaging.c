#include "messaging.h"

#include "message_keys.auto.h"
#include <pebble.h>

static const uint32_t s_inbox_size = 256;
static const uint32_t s_outbox_size = 64;

static int s_nozzle_temp = 0, s_nozzle_target = 0;
static int s_bed_temp = 0, s_bed_target = 0;
static int s_print_progress = 0, s_print_time_left = 0;
static ConnectionState s_connection_state;
static PrintState s_print_state;

static MessagingUpdateCallback s_on_update = NULL;

int messaging_get_nozzle_temp(void) {
  return s_nozzle_temp;
}
int messaging_get_nozzle_target(void) {
  return s_nozzle_target;
}
int messaging_get_bed_temp(void) {
  return s_bed_temp;
}
int messaging_get_bed_target(void) {
  return s_bed_target;
}
int messaging_get_print_progress(void) {
  return s_print_progress;
}
int messaging_get_print_time_left(void) {
  return s_print_time_left;
}
ConnectionState messaging_get_connection_state(void) {
  return s_connection_state;
}
PrintState messaging_get_print_state(void) {
  return s_print_state;
}

static void prv_inbox_received_callback(DictionaryIterator *iter, void *context) {
  Tuple *t;

  t = dict_find(iter, MESSAGE_KEY_NozzleTemp);
  if (t) {
    s_nozzle_temp = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_NozzleTarget);
  if (t) {
    s_nozzle_target = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_BedTemp);
  if (t) {
    s_bed_temp = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_BedTarget);
  if (t) {
    s_bed_target = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_PrintProgress);
  if (t) {
    s_print_progress = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_PrintTimeLeft);
  if (t) {
    s_print_time_left = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_ConnectionState);
  if (t) {
    s_connection_state = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_PrintState);
  if (t) {
    s_print_state = t->value->int32;
  }

  if (s_on_update) {
    s_on_update();
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
