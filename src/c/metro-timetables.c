#include <pebble.h>
#include <stdint.h>
#include <string.h>

#include "message_keys.auto.h"

#define STATION_TEXT_GAP 14

/* Display artifacts */
static Window* welcome_window;
static TextLayer* welcome_text_layer;
static char welcome_text[] = "WMATA Metro\nTimetables";

static Window* station_window;
static MenuLayer* station_menu_layer;

static Window* trains_window;
static TextLayer* trains_text_layer;
static TextLayer* trains_title_layer;

static char train_text[64];
static char current_station[64];

static GBitmap* s_bitmap;

/* ===== */

/* Data to and from watch */
static bool s_js_ready;
static char station_text[32];
static size_t stations_len;
static char** stations;
static size_t favorite_stations_len = 0;
static char* favorite_stations[5]   = {NULL};
/* ===== */

// #ifdef PBL_ROUND
static int16_t get_cell_height_callback(MenuLayer* menu_layer, MenuIndex* cell_index, void* callback_context) {
  return 60;
}
// #endif

/* ======================= Trains Window ================================= */
static void get_train_data(struct MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In select callback");
  DictionaryIterator* out_iter;

  AppMessageResult result = app_message_outbox_begin(&out_iter);

  if (result == APP_MSG_OK) {
    strcpy(current_station, stations[cell_index->row]);
    dict_write_cstring(out_iter, MESSAGE_KEY_TrainRequest, current_station);
    result = app_message_outbox_send();

    if (result != APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
    }
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error initializing the message outbox: %d", (int)result);
  }
}

static void trains_window_load() {
  Layer* window_layer = window_get_root_layer(trains_window);
  GRect bounds        = layer_get_bounds(window_layer);
  GRect title_bounds  = GRect(0, 0, bounds.size.w, bounds.size.h / 8);
  GRect text_bounds   = GRect(0, bounds.size.h / 5, bounds.size.w, bounds.size.h);

  trains_title_layer = text_layer_create(title_bounds);
  text_layer_set_text(trains_title_layer, current_station);
  text_layer_set_background_color(trains_title_layer, GColorBlack);
  text_layer_set_text_color(trains_title_layer, GColorWhite);
  text_layer_set_text_alignment(trains_title_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(trains_title_layer));

  trains_text_layer = text_layer_create(text_bounds);
  text_layer_set_text(trains_text_layer, train_text);
  layer_add_child(window_layer, text_layer_get_layer(trains_text_layer));
}

static void trains_window_unload(Window* window) {
  text_layer_destroy(trains_text_layer);
  text_layer_destroy(trains_title_layer);
}

static void init_trains_window() {
  trains_window = window_create();
  window_set_window_handlers(
      trains_window,
      (WindowHandlers){
          .load   = trains_window_load,
          .unload = trains_window_unload,
      }
  );
}

static void draw_row_handler(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* callback_context) {
  char* name        = stations[cell_index->row];
  int text_gap_size = STATION_TEXT_GAP - strlen(name);

  // Using simple space padding between name and station_text for appearance
  // of edge-alignment
  snprintf(station_text, sizeof(station_text), "%s", PBL_IF_ROUND_ELSE("", name));
  menu_cell_basic_draw(
      ctx, cell_layer, PBL_IF_ROUND_ELSE(name, station_text), PBL_IF_ROUND_ELSE(station_text, NULL), NULL
  );
}

static uint16_t get_sections_count_callback(
    struct MenuLayer* menulayer, uint16_t section_index, void* callback_context
) {
  int count = stations_len;
  return count;
}

void process_tuple(Tuple* t) {
  uint32_t key = t->key;

  int value = t->value->int32;

  if (key == MESSAGE_KEY_JSReady) {
    s_js_ready = true;
  }
  else if (key == MESSAGE_KEY_StationsLen) {
    stations_len = value;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received %d stations", (int)stations_len);
    stations = malloc(stations_len * sizeof(char*));
  }
  else if (key >= MESSAGE_KEY_Stations && key <= (MESSAGE_KEY_Stations + (int)stations_len)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received station %d: %s", (int)key, t->value->cstring);
    stations[key - MESSAGE_KEY_Stations] = malloc(strlen(t->value->cstring) + 1);
    strcpy(stations[key - MESSAGE_KEY_Stations], t->value->cstring);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Stored station %d: %s", (int)key, stations[key - MESSAGE_KEY_Stations]);
  }
  else if (key == MESSAGE_KEY_TrainResponse) {
    strcpy(train_text, (char*)t->value->data);
    window_stack_push(trains_window, true);
  }
  else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Key %d not recognized!", (int)key);
  }

  if (key == (MESSAGE_KEY_Stations + (int)stations_len) - 1) {
    // invalidate the layer so it redraws
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Invalidating menu layer");
  }
  return;
}

/* Message Handlers */
static void inbox_received_handler(DictionaryIterator* iter, void* context) {
  Tuple* t = dict_read_first(iter);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Inbox received handler: %d", (int)t->key);

  while (t != NULL) {
    process_tuple(t);
    t = dict_read_next(iter);
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void* context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}

static void outbox_sent_handler(DictionaryIterator* iter, void* context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message sent successfully.");
}

static void outbox_failed_handler(DictionaryIterator* iter, AppMessageResult reason, void* context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message send failed. Reason: %d", (int)reason);
}
/* ===== */

static void logo_update_proc(Layer* layer, GContext* ctx) {
  GRect bitmap_bounds    = gbitmap_get_bounds(s_bitmap);
  bitmap_bounds.origin.x = (layer_get_frame(layer).size.w - bitmap_bounds.size.w) / 2;

  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_bitmap, bitmap_bounds);
}

static size_t get_len(char** arr) {
  size_t size = 0;
  while (size < 5 && favorite_stations[size] != NULL) {
    size++;
  }
  return size;
}

static void set_unset_favorite_station(struct MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In set_unset_favorite_station");
  DictionaryIterator* out_iter;
  AppMessageResult result = app_message_outbox_begin(&out_iter);
  favorite_stations_len   = get_len(favorite_stations);

  if (result == APP_MSG_OK) {
    uint32_t action = MESSAGE_KEY_AddFavorite;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "There are %zu favorite stations.", favorite_stations_len);
    char* candidate_station = stations[cell_index->row];
    size_t i                = 0;
    while (i < favorite_stations_len) {
      if (strcmp(candidate_station, favorite_stations[i])) {
        action = MESSAGE_KEY_RemoveFavorite;
        break;
      }
      ++i;
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "There are now %zu favorite stations.", favorite_stations_len);
    dict_write_cstring(out_iter, action, stations[cell_index->row]);
    result = app_message_outbox_send();

    if (result != APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
    }
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error initializing the message outbox: %d", (int)result);
  }
}

static void station_window_load() {
  Layer* window_layer = window_get_root_layer(station_window);
  GRect bounds        = layer_get_bounds(window_layer);
  station_menu_layer  = menu_layer_create(bounds);

  menu_layer_set_callbacks(
      station_menu_layer, NULL,
      (MenuLayerCallbacks){.get_num_rows      = get_sections_count_callback,
                           .get_cell_height   = get_cell_height_callback,
                           .draw_row          = draw_row_handler,
                           .select_click      = get_train_data,
                           .select_long_click = set_unset_favorite_station}
  );
  menu_layer_set_click_config_onto_window(station_menu_layer, station_window);
  layer_add_child(window_layer, menu_layer_get_layer(station_menu_layer));
}

static void station_window_unload(Window* window) { menu_layer_destroy(station_menu_layer); }

static void init_station_window() {
  station_window = window_create();
  window_set_window_handlers(
      station_window,
      (WindowHandlers){
          .load   = station_window_load,
          .unload = station_window_unload,
      }
  );
}

/* ======================= Welcome Window ================================= */

static void welcome_window_load() {
  Layer* window_layer = window_get_root_layer(welcome_window);
  GRect bounds        = layer_get_bounds(window_layer);

  welcome_text_layer =
      text_layer_create(GRect(0, bounds.size.h - (bounds.size.h / 4) - 4, bounds.size.w, bounds.size.h / 6));
  text_layer_set_text(welcome_text_layer, welcome_text);
  text_layer_set_background_color(welcome_text_layer, GColorClear);
  text_layer_set_text_alignment(welcome_text_layer, GTextAlignmentCenter);

  layer_set_update_proc(window_layer, logo_update_proc);
  layer_add_child(window_layer, text_layer_get_layer(welcome_text_layer));
}

static void welcome_window_unload(Window* window) { text_layer_destroy(welcome_text_layer); }

static void welcome_select_click_handler(ClickRecognizerRef recognizer, void* context) {
  window_stack_push(station_window, true);
}

static void welcome_window_config_provider(void* context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, welcome_select_click_handler);
}

static void init_welcome_window() {
  welcome_window = window_create();
  window_set_click_config_provider(welcome_window, welcome_window_config_provider);
  window_set_window_handlers(
      welcome_window,
      (WindowHandlers){
          .load   = welcome_window_load,
          .unload = welcome_window_unload,
      }
  );
  window_stack_push(welcome_window, true);
}

static void prv_init(void) {
  init_welcome_window();
  init_station_window();
  init_trains_window();
  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO);

  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_open(400, 400);
}

static void prv_deinit(void) {
  window_destroy(trains_window);
  window_destroy(station_window);
  window_destroy(welcome_window);

  for (uint32_t i = 0; i < stations_len; i++) {
    free(stations[i]);
  }
  free(stations);

  gbitmap_destroy(s_bitmap);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
