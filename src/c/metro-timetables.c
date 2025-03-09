#include <pebble.h>
#include <stdint.h>

#include "message_keys.auto.h"

#define STATION_TEXT_GAP 14

/* Display artifacts */
static Window* welcome_window;
static TextLayer* welcome_text_layer;

static Window* station_window;
static MenuLayer* station_menu_layer;

static Window* trains_window;
static ScrollLayer* trains_scroll_layer;
static TextLayer* trains_text_layer;

static char scroll_text[] = "RD Glenmont 3mins\nRD Shady Grove 5mins\nRD Glenmont 9mins";
/* ===== */

/* Data to and from watch */
static bool s_js_ready;
static char station_text[32];
static uint32_t favorite_stations_len;
static char** favorite_stations;
/* ===== */

// #ifdef PBL_ROUND
static int16_t get_cell_height_callback(MenuLayer* menu_layer, MenuIndex* cell_index, void* callback_context) {
  return 60;
}
// #endif

/* ======================= Trains Window ================================= */
static void trains_window_load() {
  Layer* window_layer   = window_get_root_layer(trains_window);
  GRect bounds          = layer_get_bounds(window_layer);
  GRect max_text_bounds = GRect(0, 0, bounds.size.w, 2000);

  trains_scroll_layer = scroll_layer_create(bounds);
  scroll_layer_set_click_config_onto_window(trains_scroll_layer, trains_window);

  // Initialize the text layer
  trains_text_layer = text_layer_create(max_text_bounds);
  text_layer_set_text(trains_text_layer, scroll_text);
  scroll_layer_add_child(trains_scroll_layer, text_layer_get_layer(trains_text_layer));
  layer_add_child(window_layer, scroll_layer_get_layer(trains_scroll_layer));
}

static void trains_window_unload(Window* window) {
  text_layer_destroy(trains_text_layer);
  scroll_layer_destroy(trains_scroll_layer);
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
  const bool animated = true;
  window_stack_push(trains_window, animated);
}

/* ======================= Stations Window ================================= */

static void draw_row_handler(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* callback_context) {
  char* name        = favorite_stations[cell_index->row];
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
  int count = favorite_stations_len;
  return count;
}

/* Helpers */
void test_send() {
  DictionaryIterator* out_iter;

  AppMessageResult result = app_message_outbox_begin(&out_iter);

  if (result == APP_MSG_OK) {
    char* value = "A03";
    dict_write_cstring(out_iter, MESSAGE_KEY_TrainRequest, value);
    result = app_message_outbox_send();

    if (result != APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
    }
  }
  else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error initializing the message outbox: %d", (int)result);
  }
}

void process_tuple(Tuple* t) {
  uint32_t key = t->key;

  int value = t->value->int32;

  if (key == MESSAGE_KEY_JSReady) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "WOO GOT READY: %d", 100);
    s_js_ready = true;
    test_send();
  }
  else if (key == MESSAGE_KEY_FavoriteStationsLen) {
    favorite_stations_len = value;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received %d stations", (int)favorite_stations_len);
    favorite_stations = malloc(favorite_stations_len * sizeof(char*));
  }
  else if (key >= MESSAGE_KEY_FavoriteStations && key <= (MESSAGE_KEY_FavoriteStations + (int)favorite_stations_len)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received station %d: %s", (int)key, t->value->cstring);
    favorite_stations[key - MESSAGE_KEY_FavoriteStations] = malloc(strlen(t->value->cstring) + 1);
    strcpy(favorite_stations[key - MESSAGE_KEY_FavoriteStations], t->value->cstring);
    APP_LOG(
        APP_LOG_LEVEL_DEBUG, "Stored station %d: %s", (int)key, favorite_stations[key - MESSAGE_KEY_FavoriteStations]
    );
  }
  else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Key %d not recognized!", (int)key);
  }

  if (key == (MESSAGE_KEY_FavoriteStations + (int)favorite_stations_len) - 1) {
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

static void station_window_load() {
  Layer* window_layer = window_get_root_layer(station_window);
  GRect bounds        = layer_get_bounds(window_layer);
  station_menu_layer  = menu_layer_create(bounds);

  menu_layer_set_callbacks(
      station_menu_layer, NULL,
      (MenuLayerCallbacks){.get_num_rows    = get_sections_count_callback,
                           .get_cell_height = get_cell_height_callback,
                           .draw_row        = draw_row_handler,
                           .select_click    = init_trains_window}
  );
  menu_layer_set_click_config_onto_window(station_menu_layer, station_window);
  layer_add_child(window_layer, menu_layer_get_layer(station_menu_layer));
}

static void station_window_unload(Window* window) { menu_layer_destroy(station_menu_layer); }

static void prv_select_click_handler(ClickRecognizerRef recognizer, void* context) {}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void* context) {}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void* context) {}

static void station_window_click_config_provider(void* context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void init_station_window() {
  station_window = window_create();
  window_set_click_config_provider(station_window, station_window_click_config_provider);
  window_set_window_handlers(
      station_window,
      (WindowHandlers){
          .load   = station_window_load,
          .unload = station_window_unload,
      }
  );
  const bool animated = true;
  window_stack_push(station_window, animated);
}

/* ======================= Welcome Window ================================= */

static void welcome_window_load() {
  Layer* window_layer = window_get_root_layer(welcome_window);
  GRect bounds        = layer_get_bounds(window_layer);
  welcome_text_layer  = text_layer_create(GRect(0, 50, bounds.size.w, 20));

  text_layer_set_text(welcome_text_layer, "WMATA Metro\nTimetables");
  text_layer_set_text_alignment(welcome_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(welcome_text_layer));
}

static void welcome_window_unload(Window* window) { text_layer_destroy(welcome_text_layer); }

static void welcome_select_click_handler(ClickRecognizerRef recognizer, void* context) { init_station_window(); }

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
  const bool animated = true;
  window_stack_push(welcome_window, animated);
}

static void prv_init(void) {
  init_welcome_window();

  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void prv_deinit(void) {
  window_destroy(station_window);
  window_destroy(trains_window);
  window_destroy(welcome_window);

  for (uint32_t i = 0; i < favorite_stations_len; i++) {
    free(favorite_stations[i]);
  }
  free(favorite_stations);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
