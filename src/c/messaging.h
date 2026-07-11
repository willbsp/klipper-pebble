#pragma once

#include <pebble.h>

typedef enum {
  CONN_UNKNOWN = 0,
  CONN_OK = 1,
  CONN_ERROR = 2,
  CONN_UNREACHABLE = 3,
  CONN_NOT_CONFIGURED = 4
} ConnectionState;

typedef enum {
  PRINT_UNKNOWN = 0,
  PRINT_STANDBY = 1,
  PRINT_PRINTING = 2,
  PRINT_PAUSED = 3,
  PRINT_COMPLETE = 4,
  PRINT_CANCELLED = 5,
  PRINT_ERROR = 6,
} PrintState;

typedef void (*MessagingUpdateCallback)(void);

void messaging_init(MessagingUpdateCallback on_update);

int messaging_get_nozzle_temp(void);
int messaging_get_nozzle_target(void);
int messaging_get_bed_temp(void);
int messaging_get_bed_target(void);
int messaging_get_print_progress(void);
int messaging_get_print_time_left(void);
ConnectionState messaging_get_connection_state(void);
PrintState messaging_get_print_state(void);
