#include "cards_window.h"
#include "dialog_config_window.h"
#include "messaging.h"
#include <pebble.h>

static void prv_on_messaging_update(void) {
  if (messaging_get_connection_state() == CONN_NOT_CONFIGURED) {
    dialog_config_window_show();
  } else {
    dialog_config_window_hide();
    cards_window_update();
  }
}

static void prv_init(void) {
  cards_window_init();
  messaging_init(prv_on_messaging_update);
}

int main(void) {
  prv_init();
  app_event_loop();
}
