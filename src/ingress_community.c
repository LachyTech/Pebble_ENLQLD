#include "pebble.h"
#include <string.h>

// Text maximum lengths
#define COMMUNITY_LENGTH 17
#define MOTTO_LENGTH 18
// Faction values
#define FACTION_ENL 0
#define FACTION_RES 1
// Parameter magic numbers
#define COMMUNITY_MAGIC 0
#define MOTTO_MAGIC 1
#define FACTION_MAGIC 2

static Window *s_main_window;
static TextLayer *community_text_layer, *motto_text_layer, *s_date_layer, *s_time_layer;
static Layer *s_line_layer_h;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
// customisation options
static int faction_select;
static char *community_content, *motto_content;

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
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  
  // Add community
  community_text_layer = text_layer_create(GRect(5, 5, 140, 24));
  text_layer_set_text_color(community_text_layer, GColorWhite);
  text_layer_set_text(community_text_layer, community_content);
  text_layer_set_background_color(community_text_layer, GColorClear);
  text_layer_set_font(community_text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  layer_add_child(window_layer, text_layer_get_layer(community_text_layer));
  
  // Add motto
  motto_text_layer = text_layer_create(GRect(5, 33, 140, 20));
  text_layer_set_text(motto_text_layer, motto_content);
  text_layer_set_text_color(motto_text_layer, GColorWhite);
  text_layer_set_background_color(motto_text_layer, GColorClear);
  text_layer_set_font(motto_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_layer, text_layer_get_layer(motto_text_layer));

  // Add Date
  s_date_layer = text_layer_create(GRect(12, 142, 136, 30));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  // Add Time
  s_time_layer = text_layer_create(GRect(10, 92, 139, 50));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Add horizontal line
  GRect line_frame_h = GRect(5, 31, 140, 2);
  s_line_layer_h = layer_create(line_frame_h);
  layer_set_update_proc(s_line_layer_h, line_layer_update_callback);
  layer_add_child(window_layer, s_line_layer_h);
}

static void main_window_unload(Window *window) {
  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);

  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(community_text_layer);
  text_layer_destroy(motto_text_layer);

  layer_destroy(s_line_layer_h);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
  // read first item
  Tuple *t = dict_read_first(iterator);
  // while items exist
  while(t) {
    switch(t->key) {
    case COMMUNITY_MAGIC:
      strncpy(community_content, t->value->cstring, COMMUNITY_LENGTH -1);
      break;
    case MOTTO_MAGIC:
      strncpy(motto_content, t->value->cstring, MOTTO_LENGTH -1);
      break;
    case FACTION_MAGIC:
      if (strcmp(t->value->cstring, "0") == 0)
        faction_select = FACTION_ENL;
      else if (strcmp(t->value->cstring, "1") == 0)
        faction_select = FACTION_RES;
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }
    // Look for next item
    t = dict_read_next(iterator);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void set_faction(int faction) {
  if (s_background_bitmap)
    gbitmap_destroy(s_background_bitmap);  // destroy existing bitmap if exists
  if (faction == FACTION_ENL) {
      faction_select = FACTION_ENL;
      s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ENL_LOGO);
    } else if (faction == FACTION_RES) {
      faction_select = FACTION_RES;
      s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_RES_LOGO);
    } else {
      APP_LOG(APP_LOG_LEVEL_WARNING, "Unknown faction, setting default");
      faction_select = FACTION_ENL;
      s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ENL_LOGO);
    }
}

static void configure_watchapp() {
  // allocate configuration buffers
  community_content = malloc(sizeof(char) * COMMUNITY_LENGTH);
  motto_content = malloc(sizeof(char) * MOTTO_LENGTH);
  // load persisted values if exist
  if (persist_exists(COMMUNITY_MAGIC) || persist_exists(MOTTO_MAGIC) || persist_exists(FACTION_MAGIC)) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Loading config from cache");
    persist_read_string(COMMUNITY_MAGIC, community_content, COMMUNITY_LENGTH);
    persist_read_string(MOTTO_MAGIC, motto_content, MOTTO_LENGTH);
    set_faction(persist_read_int(FACTION_MAGIC));
  } else { // load defaults
    APP_LOG(APP_LOG_LEVEL_INFO, "Custom settings not found, loading defaults for ENL QLD");
    strncpy(community_content, "Enlightened QLD", COMMUNITY_LENGTH);
    community_content[COMMUNITY_LENGTH-1] = '\0';
    strncpy(motto_content, "For the Shiggles!", MOTTO_LENGTH);
    community_content[MOTTO_LENGTH-1] = '\0';
    set_faction(FACTION_ENL);
  }
}

static void init() {
  // configure watchapp
  configure_watchapp();
  // register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  // create windows
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
  
  // persist customisations to cache
  persist_write_string(COMMUNITY_MAGIC, community_content);
  persist_write_string(MOTTO_MAGIC, motto_content);
  persist_write_int(FACTION_MAGIC, faction_select);
  
  free(community_content);
  free(motto_content);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
