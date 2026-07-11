#include "cards_window.h"
#include "dialog_status_window.h"
#include "messaging.h"
#include <pebble.h>

typedef enum {
  APP_ERROR_NONE = 0,
  APP_ERROR_NO_PHONE,
  APP_ERROR_NOT_CONFIGURED,
  APP_ERROR_NO_MOONRAKER,
} AppError;

static const struct {
  const char *title;
  const char *message;
  uint32_t icon;
} s_dialog_content[] = {
    [APP_ERROR_NO_PHONE] = {"No connection", "Can't reach your phone",
                            RESOURCE_ID_WATCH_DISCONNECT},
    [APP_ERROR_NOT_CONFIGURED] = {"Configuration required",
                                  "Set up printer connection in the Pebble app",
                                  RESOURCE_ID_CONFIG_REQUIRED},
    [APP_ERROR_NO_MOONRAKER] = {"Connection failed", "Failed to connect to Moonraker",
                                RESOURCE_ID_NO_INTERNET},
};

static AppError s_last_error;

static AppError prv_current_error(void) {
  if (!connection_service_peek_pebble_app_connection()) {
    return APP_ERROR_NO_PHONE;
  }
  switch (messaging_get_connection_state()) {
  case CONN_NOT_CONFIGURED:
    return APP_ERROR_NOT_CONFIGURED;
  case CONN_ERROR:
  case CONN_UNREACHABLE:
    return APP_ERROR_NO_MOONRAKER;
  default:
    return APP_ERROR_NONE;
  }
}

static void prv_refresh_ui(void) {
  AppError error = prv_current_error();
  if (!error) {
    dialog_config_window_hide();
    cards_window_update();
    return;
  }
  if (error != s_last_error) {
    dialog_config_window_hide();
  }
  dialog_config_window_show(s_dialog_content[error].title, s_dialog_content[error].message,
                            s_dialog_content[error].icon);
  s_last_error = error;
}

static void prv_pebble_app_connection_handler(bool connected) {
  prv_refresh_ui();
}

static void prv_on_messaging_update_callback(void) {
  prv_refresh_ui();
}

static void prv_init(void) {
  cards_window_init();
  connection_service_subscribe(
      (ConnectionHandlers){.pebble_app_connection_handler = prv_pebble_app_connection_handler});
  messaging_init(prv_refresh_ui);
  prv_refresh_ui();
}

int main(void) {
  prv_init();
  app_event_loop();
}
