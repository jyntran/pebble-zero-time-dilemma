#include "zero-time-dilemma.h"
#include "settings.h"

static void prv_default_settings() {
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));

  prv_window_update();
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {  
  prv_save_settings();
}

void prv_load_settings() {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));

  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
}
