#include "pebble.h"

static Window *s_main_window;
static TextLayer *ingress_text_layer, *shiggles_text_layer, *s_date_layer, *s_time_layer;
static Layer *s_line_layer_h, *s_line_layer_v;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char s_time_text[] = "00:00";
  static char s_date_text[] = "Xxxxxxxxx 00";

  strftime(s_date_text, sizeof(s_date_text), "%e %B", tick_time);
  text_layer_set_text(s_date_layer, s_date_text);

  char *time_format;
  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }
  strftime(s_time_text, sizeof(s_time_text), time_format, tick_time);

  // Handle lack of non-padded hour format string for twelve hour clock.
  if (!clock_is_24h_style() && (s_time_text[0] == '0')) {
    memmove(s_time_text, &s_time_text[1], sizeof(s_time_text) - 1);
  }
  text_layer_set_text(s_time_layer, s_time_text);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  // Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ENL_LOGO);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  
  ingress_text_layer = text_layer_create(GRect(5, 5, 140, 24));
  text_layer_set_text(ingress_text_layer, "Enlightened QLD");
  text_layer_set_text_color(ingress_text_layer, GColorWhite);
  text_layer_set_background_color(ingress_text_layer, GColorClear);
  text_layer_set_font(ingress_text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  layer_add_child(window_layer, text_layer_get_layer(ingress_text_layer));
  
  shiggles_text_layer = text_layer_create(GRect(5, 33, 140, 20));
  text_layer_set_text(shiggles_text_layer, "For the Shiggles!");
  text_layer_set_text_color(shiggles_text_layer, GColorWhite);
  text_layer_set_background_color(shiggles_text_layer, GColorClear);
  text_layer_set_font(shiggles_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_layer, text_layer_get_layer(shiggles_text_layer));

  s_date_layer = text_layer_create(GRect(12, 142, 136, 30));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  s_time_layer = text_layer_create(GRect(10, 92, 139, 50));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  GRect line_frame_h = GRect(5, 31, 140, 2);
  s_line_layer_h = layer_create(line_frame_h);
  layer_set_update_proc(s_line_layer_h, line_layer_update_callback);
  layer_add_child(window_layer, s_line_layer_h);
  
  GRect line_frame_v = GRect(85, 29, 2, 139);
  s_line_layer_v = layer_create(line_frame_v);
  layer_set_update_proc(s_line_layer_v, line_layer_update_callback);
  //layer_add_child(window_layer, s_line_layer_v);
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(ingress_text_layer);

  layer_destroy(s_line_layer_h);
  layer_destroy(s_line_layer_v);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  
  // Prevent starting blank
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  handle_minute_tick(t, MINUTE_UNIT);
}

static void deinit() {
  window_destroy(s_main_window);

  tick_timer_service_unsubscribe();
}

int main() {
  init();
  app_event_loop();
  deinit();
}
