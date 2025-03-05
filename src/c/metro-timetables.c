#include <pebble.h>

#define STATION_TEXT_GAP 14

static Window *s_window;
static TextLayer *s_text_layer;
static MenuLayer *s_menu_layer;
static bool s_js_ready;
static char station_text[32];

static uint32_t favorite_stations_len;
static char **favorite_stations;

#ifdef PBL_ROUND
static int16_t get_cell_height_callback(MenuLayer *menu_layer,
                                        MenuIndex *cell_index,
                                        void *callback_context) {
  return 60;
}
#endif

typedef struct {
  char name[32]; // Name of this station
} Station;

// Array of different stations
// {<Station Name>, <Lines OR'd together>}
Station station_array[] = {
    {"Dupont"},
    {"Court House"},
    {"Rosslyn"},
    {"Metro Center"},
};

static void draw_row_handler(GContext *ctx, const Layer *cell_layer,
                             MenuIndex *cell_index, void *callback_context) {
  char *name = favorite_stations[cell_index->row];
  int text_gap_size = STATION_TEXT_GAP - strlen(name);

  // Using simple space padding between name and station_text for appearance of
  // edge-alignment
  snprintf(station_text, sizeof(station_text), "%s",
           PBL_IF_ROUND_ELSE("", name));
  menu_cell_basic_draw(ctx, cell_layer, PBL_IF_ROUND_ELSE(name, station_text),
                       PBL_IF_ROUND_ELSE(station_text, NULL), NULL);
}

static uint16_t get_sections_count_callback(struct MenuLayer *menulayer,
                                            uint16_t section_index,
                                            void *callback_context) {
  int count = favorite_stations_len;
  return count;
}

static void prv_select_click_handler(ClickRecognizerRef recognizer,
                                     void *context) {
  text_layer_set_text(s_text_layer, "Select");
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Up");
}

static void prv_down_click_handler(ClickRecognizerRef recognizer,
                                   void *context) {
  text_layer_set_text(s_text_layer, "Down");
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
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
      (MenuLayerCallbacks){
          .get_num_rows = get_sections_count_callback,
          .get_cell_height = PBL_IF_ROUND_ELSE(get_cell_height_callback, NULL),
          .draw_row = draw_row_handler,
          .select_click = NULL});
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
}

static void load_window() {
  s_window = window_create();
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = prv_window_load,
                                           .unload = prv_window_unload,
                                       });
  const bool animated = true;
  window_stack_push(s_window, animated);
}

void process_tuple(Tuple *t) {
  uint32_t key = t->key;

  int value = t->value->int32;

  if (key == MESSAGE_KEY_JSReady) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "WOO GOT READY: %d", 100);
    s_js_ready = true;
  } else if (key == MESSAGE_KEY_FavoriteStationsLen) {
    favorite_stations_len = value;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received %d stations",
            (int)favorite_stations_len);
    favorite_stations = malloc(favorite_stations_len * sizeof(char *));
  } else if (key >= MESSAGE_KEY_FavoriteStations &&
             key <=
                 (MESSAGE_KEY_FavoriteStations + (int)favorite_stations_len)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received station %d: %s", (int)key,
            t->value->cstring);
    favorite_stations[key - MESSAGE_KEY_FavoriteStations] =
        malloc(strlen(t->value->cstring) + 1);
    strcpy(favorite_stations[key - MESSAGE_KEY_FavoriteStations],
           t->value->cstring);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Stored station %d: %s", (int)key,
            favorite_stations[key - MESSAGE_KEY_FavoriteStations]);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Key %d not recognized!", (int)key);
  }

  if (key == (MESSAGE_KEY_FavoriteStations + (int)favorite_stations_len) - 1) {
      // invalidate the layer so it redraws
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Invalidating menu layer");
      layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
  }
  return;
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *t = dict_read_first(iter);

  while (t != NULL) {
    process_tuple(t);
    t = dict_read_next(iter);
  }
}

static void prv_init(void) {
  load_window();
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(),
                   app_message_outbox_size_maximum());
}

static void prv_deinit(void) { window_destroy(s_window); }

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p",
          s_window);

  app_event_loop();
  prv_deinit();
}
