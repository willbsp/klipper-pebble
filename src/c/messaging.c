#include "messaging.h"

#include "message_keys.auto.h"
#include "persist_keys.h"
#include <pebble.h>

static const uint32_t s_inbox_size = 256;
static const uint32_t s_outbox_size = 64;

struct PrinterState {
  int nozzle_temp, nozzle_target, bed_temp, bed_target;
  int print_progress, print_time_left;
  PrintState print_state;
};

static struct PrinterState s_printer_state;

static ConnectionState s_connection_state;

static MessagingUpdateCallback s_on_update = NULL;

int messaging_get_nozzle_temp(void) {
  return s_printer_state.nozzle_temp;
}
int messaging_get_nozzle_target(void) {
  return s_printer_state.nozzle_target;
}
int messaging_get_bed_temp(void) {
  return s_printer_state.bed_temp;
}
int messaging_get_bed_target(void) {
  return s_printer_state.bed_target;
}
int messaging_get_print_progress(void) {
  return s_printer_state.print_progress;
}
int messaging_get_print_time_left(void) {
  return s_printer_state.print_time_left;
}
PrintState messaging_get_print_state(void) {
  return s_printer_state.print_state;
}
ConnectionState messaging_get_connection_state(void) {
  return s_connection_state;
}

static void prv_save_printer_state(void) {
  int result =
      persist_write_data(PERSIST_KEY_PRINTER_STATE, &s_printer_state, sizeof(struct PrinterState));
  if (result < 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Persisting data was unsuccessful. Got status %d", result);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Persist wrote %d bytes", result);
  }
}

static void prv_restore_printer_state(void) {
  if (persist_exists(PERSIST_KEY_PRINTER_STATE)) {
    int result =
        persist_read_data(PERSIST_KEY_PRINTER_STATE, &s_printer_state, sizeof(struct PrinterState));
    if (result < 0) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Retrieving data was unsuccessful. Got status %d", result);
    } else {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Persist read %d bytes", result);
    }
  }
}

static void prv_inbox_received_callback(DictionaryIterator *iter, void *context) {
  Tuple *t;

  t = dict_find(iter, MESSAGE_KEY_NozzleTemp);
  if (t) {
    s_printer_state.nozzle_temp = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_NozzleTarget);
  if (t) {
    s_printer_state.nozzle_target = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_BedTemp);
  if (t) {
    s_printer_state.bed_temp = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_BedTarget);
  if (t) {
    s_printer_state.bed_target = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_PrintProgress);
  if (t) {
    s_printer_state.print_progress = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_PrintTimeLeft);
  if (t) {
    s_printer_state.print_time_left = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_PrintState);
  if (t) {
    s_printer_state.print_state = t->value->int32;
  }

  t = dict_find(iter, MESSAGE_KEY_ConnectionState);
  if (t) {
    s_connection_state = t->value->int32;
  }

  prv_save_printer_state();

  if (s_on_update) {
    s_on_update();
  }
}

static void prv_inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
}

void messaging_init(MessagingUpdateCallback on_update) {
  s_on_update = on_update;
  prv_restore_printer_state();
  app_message_register_inbox_received(prv_inbox_received_callback);
  app_message_register_inbox_dropped(prv_inbox_dropped_callback);
  app_message_open(s_inbox_size, s_outbox_size);
}
