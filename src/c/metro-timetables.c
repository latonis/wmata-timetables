#include <pebble.h>
#include <stdint.h>

#include "message_keys.auto.h"

#define STATION_TEXT_GAP 14

/* Display artifacts */
static Window* station_window;
static Window* trains_window;
static TextLayer* s_text_layer;
static MenuLayer* s_menu_layer;
static ScrollLayer* s_scroll_layer;
static TextLayer* s_text_scroll_layer;

static char scroll_text[] = "RD Glenmont 3mins\nRD Shady Grove 5mins\nRD Glenmont 9mins";
/* ===== */

/* Data to and from watch */
static bool s_js_ready;
static char station_text[32];
static uint32_t favorite_stations_len;
static char** favorite_stations;
/* ===== */

#ifdef PBL_ROUND
static int16_t get_cell_height_callback(MenuLayer* menu_layer, MenuIndex* cell_index, void* callback_context) {
  return 60;
}
#endif

static void prv_select_click_handler(ClickRecognizerRef recognizer, void* context) {
  text_layer_set_text(s_text_layer, "Select");
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void* context) {
  text_layer_set_text(s_text_layer, "Up");
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void* context) {
  text_layer_set_text(s_text_layer, "Down");
}

static void prv_click_config_provider(void* context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

/* ======================= Button Handlers ================================= */
static void trains_window_load() {
  Layer* window_layer = window_get_root_layer(trains_window);
  GRect bounds = layer_get_bounds(window_layer);
  GRect max_text_bounds = GRect(0, 0, bounds.size.w, 2000);

  s_scroll_layer = scroll_layer_create(bounds);
  scroll_layer_set_click_config_onto_window(s_scroll_layer, trains_window);

  // Initialize the text layer
  s_text_layer = text_layer_create(max_text_bounds);
  text_layer_set_text(s_text_layer, scroll_text);
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));
  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));
}

static void trains_window_unload(Window* window) {
  text_layer_destroy(s_text_layer);
  scroll_layer_destroy(s_scroll_layer);
}

static void trains_window_init() {
  trains_window = window_create();
  window_set_click_config_provider(trains_window, prv_click_config_provider);
  window_set_window_handlers(
      trains_window,
      (WindowHandlers){
          .load = trains_window_load,
          .unload = trains_window_unload,
      }
  );
  const bool animated = true;
  window_stack_push(trains_window, animated);
}

void select_callback(struct MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In select callback");
  trains_window_init();
}

/* ======================================================================== */

static void draw_row_handler(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* callback_context) {
  char* name = favorite_stations[cell_index->row];
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

static void prv_window_load(Window* window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_text_layer = text_layer_create(GRect(0, 50, bounds.size.w, 20));
  text_layer_set_text(s_text_layer, "WMATA Metro");
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));

  s_text_layer = text_layer_create(GRect(0, 75, bounds.size.w, 20));
  text_layer_set_text(s_text_layer, "Timetables");
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(
      s_menu_layer, NULL,
      (MenuLayerCallbacks){.get_num_rows = get_sections_count_callback,
                           .get_cell_height = PBL_IF_ROUND_ELSE(get_cell_height_callback, NULL),
                           .draw_row = draw_row_handler,
                           .select_click = select_callback}
  );
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void prv_window_unload(Window* window) { text_layer_destroy(s_text_layer); }

static void load_station_window() {
  station_window = window_create();
  window_set_click_config_provider(station_window, prv_click_config_provider);
  window_set_window_handlers(
      station_window,
      (WindowHandlers){
          .load = prv_window_load,
          .unload = prv_window_unload,
      }
  );
  const bool animated = true;
  window_stack_push(station_window, animated);
}

/* Helpers */
void test_send() {
  DictionaryIterator* out_iter;

  AppMessageResult result = app_message_outbox_begin(&out_iter);

  if (result == APP_MSG_OK) {
    char *value = "A03";
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
    layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
  }
  return;
}
/* ===== */

/* Message Handlers */
static void inbox_received_handler(DictionaryIterator* iter, void* context) {
  Tuple* t = dict_read_first(iter);

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

static void prv_init(void) {
  load_station_window();

  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void prv_deinit(void) {
  window_destroy(station_window);
  window_destroy(trains_window);
  menu_layer_destroy(s_menu_layer);

  for (uint32_t i = 0; i < favorite_stations_len; i++) {
    free(favorite_stations[i]);
  }
  free(favorite_stations);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", station_window);
  app_event_loop();
  prv_deinit();
}
