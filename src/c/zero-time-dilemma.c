#include <pebble.h> 
#include <ctype.h>
#include "zero-time-dilemma.h"
#include "settings.h"

static Window *s_window;
static TextLayer  *s_time_layer,
                  *s_date_layer,
                  *s_battery_layer;
static Layer *time_layer,
              *date_layer,
              *battery_layer;
static int s_battery_level;

static void send_app_message() {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_uint8(iter, 0, 0);
  app_message_outbox_send();
}

void prv_window_update() {
  layer_mark_dirty(time_layer);
  layer_mark_dirty(date_layer);
  layer_mark_dirty(battery_layer);
}

static void clock_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
  GRect unobstructed_bounds = layer_get_unobstructed_bounds(window_get_root_layer(s_window));
  int16_t obstruction_height = bounds.size.h - unobstructed_bounds.size.h;

  if (obstruction_height != 0) {
    bounds = unobstructed_bounds;
  }

  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);

  layer_set_frame(layer, bounds);
  layer_set_frame(text_layer_get_layer(s_time_layer),
    GRect(
      0, bounds.size.h/4 + 16, bounds.size.w, bounds.size.h/2
    )
  );
}

static void date_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
  GRect unobstructed_bounds = layer_get_unobstructed_bounds(window_get_root_layer(s_window));
  int16_t obstruction_height = bounds.size.h - unobstructed_bounds.size.h;

  if (obstruction_height != 0) {
    bounds = unobstructed_bounds;
  }

  layer_set_frame(layer, bounds);
  layer_set_frame(text_layer_get_layer(s_date_layer),
    GRect(
      0, 12, bounds.size.w, bounds.size.h/4
    )
  );

  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_date_buffer[32];
  strftime(s_date_buffer, sizeof(s_date_buffer),
    "%m.%d.%Y", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
  GRect unobstructed_bounds = layer_get_unobstructed_bounds(window_get_root_layer(s_window));
  int16_t obstruction_height = bounds.size.h - unobstructed_bounds.size.h;

  if (obstruction_height != 0) {
    bounds = unobstructed_bounds;
  }

  layer_set_frame(layer, bounds);
  layer_set_frame(text_layer_get_layer(s_battery_layer),
    GRect(
      0, bounds.size.h-32, bounds.size.w, bounds.size.h/4
    )
  );

  static char s_battery_buffer[16];
  if (s_battery_level) {
    snprintf(s_battery_buffer, sizeof(s_battery_buffer), "REM: %d%%", s_battery_level);
  }
  text_layer_set_text(s_battery_layer, s_battery_buffer);
}

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  layer_mark_dirty(date_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(time_layer);
  if (units_changed & DAY_UNIT) {
    layer_mark_dirty(date_layer);
  }
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(s_window, GColorBlack);

  time_layer = layer_create(bounds);
  date_layer = layer_create(bounds);
  battery_layer = layer_create(bounds);

  s_date_layer = text_layer_create(bounds);
  text_layer_set_font(s_date_layer, fonts_load_custom_font(resource_get_handle(FONT_SMALL)));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorLightGray);
  layer_add_child(date_layer, text_layer_get_layer(s_date_layer));

  s_time_layer = text_layer_create(bounds);
  text_layer_set_font(s_time_layer, fonts_load_custom_font(resource_get_handle(FONT_LARGE)));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorRed);
  layer_add_child(time_layer, text_layer_get_layer(s_time_layer));

  s_battery_layer = text_layer_create(bounds);
  text_layer_set_font(s_battery_layer, fonts_load_custom_font(resource_get_handle(FONT_SMALL)));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorLightGray);
  layer_add_child(battery_layer, text_layer_get_layer(s_battery_layer));

  layer_add_child(window_layer, date_layer);
  layer_add_child(window_layer, time_layer);
  layer_add_child(window_layer, battery_layer);

  layer_set_update_proc(time_layer, clock_update_proc);
  layer_set_update_proc(date_layer, date_update_proc);
  layer_set_update_proc(battery_layer, battery_update_proc);

  battery_callback(battery_state_service_peek());
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
}

static void prv_init(void) {
  prv_load_settings();

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();

  layer_destroy(time_layer);
  layer_destroy(date_layer);
  layer_destroy(battery_layer);
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
