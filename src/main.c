#include <pebble.h>

#define DISPLAY_LARGE_FONT FONT_KEY_BITHAM_30_BLACK
#define DISPLAY_LARGE_FONT_SIZE 30
#define DISPLAY_SMALL_FONT FONT_KEY_GOTHIC_14_BOLD
#define DISPLAY_SMALL_FONT_SIZE 14

#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

/*
 * It's unclear if this is useful for app releases or not:
 * http://forums.getpebble.com/discussion/10685/sdk-2-0-retain-app-log-or-remove-for-production-release
 */
#if 0
#undef APP_LOG
#define APP_LOG(...)
#endif

enum
{
  KNOTS = 1,
  BEARING = 2,
  CURRENT_KNOTS = 3,
  CURRENT_BEARING = 4,
  TIDE_CHANGE = 5,
  LATITUDE = 6,
  LONGITUDE = 7,
  CURRENTS_STATION_NAME = 8,
  TIDES_STATION_NAME = 9,

  END_MESSAGE_KEY
};

typedef struct GaugeLayerData
{
  TextLayer* data_layer;
  char* data;
} GaugeLayerData;

typedef struct Gauge
{
  const uint32_t message_key;
  const char* units_label;
  const char* type_label;

  Layer* (*units_layer_create)
      (uint32_t x, uint32_t width, uint32_t height, const char* label);
  Layer* layer;
} Gauge;

static const uint32_t javascript_interval_seconds = 30;
static const uint32_t units_width = 30;
static const bool invert_windows = true;
static const bool screenshot_mode = false;

static uint32_t debug_count;
static TextLayer* debug_layer;
static Layer* status_layer;
static Layer *gauge_layer;
static const char* debug_values[] = {"|", "/", "--", "\\", "|", "/", "--", "\\"};

#define INITIAL_GAUGE_VALUE "--"
static char current_data[END_MESSAGE_KEY][128] = {INITIAL_GAUGE_VALUE};


static TextLayer*
new_large_text_layer(void)
{
  TextLayer* text_layer = text_layer_create(
    (GRect) {.origin = {0, 0}, .size = {0, 0}});
  text_layer_set_font(text_layer, fonts_get_system_font(DISPLAY_LARGE_FONT));
  text_layer_set_text_alignment(text_layer, GTextAlignmentRight);
  return text_layer;
}

static TextLayer*
new_small_text_layer(void)
{
  TextLayer* text_layer = text_layer_create(
    (GRect) {.origin = {0, 0}, .size = {0, 0}});
  text_layer_set_font(text_layer, fonts_get_system_font(DISPLAY_SMALL_FONT));
  text_layer_set_text_alignment(text_layer, GTextAlignmentLeft);
  return text_layer;
}

static Layer*
new_middle_layer(uint32_t x, uint32_t width, uint32_t height, const char *label)
{
  TextLayer* units_layer = new_small_text_layer();

  text_layer_set_text(units_layer, label);
  layer_set_frame(
    text_layer_get_layer(units_layer),
    (GRect) {
      .origin = {x,
                 (DISPLAY_LARGE_FONT_SIZE / 2) - (DISPLAY_SMALL_FONT_SIZE / 2)},
      .size = {width, height}
    });
  return text_layer_get_layer(units_layer);
}

static Layer*
new_bottom_layer(uint32_t x, uint32_t width, uint32_t height, const char *label)
{
  TextLayer* units_layer = new_small_text_layer();

  text_layer_set_text(units_layer, label);
  layer_set_frame(
    text_layer_get_layer(units_layer),
    (GRect) {
      .origin = {x,
                 DISPLAY_LARGE_FONT_SIZE - DISPLAY_SMALL_FONT_SIZE},
      .size = {width, height}
    });
  return text_layer_get_layer(units_layer);
}

static Layer*
new_top_layer(uint32_t x, uint32_t width, uint32_t height, const char *label)
{
  TextLayer* units_layer = new_small_text_layer();

  text_layer_set_text(units_layer, label);
  layer_set_frame(
    text_layer_get_layer(units_layer),
    (GRect) {
      .origin = {x, 0},
      .size = {width, height}
    });
  return text_layer_get_layer(units_layer);
}

static void
gauge_update_proc(Layer* layer, GContext* ctx)
{
  GaugeLayerData* layer_data = layer_get_data(layer);
  text_layer_set_text(layer_data->data_layer, layer_data->data);
}


static Layer*
gauge_layer_create(GRect bounds, Gauge* gauge)
{
  Layer* layer = layer_create_with_data(bounds, sizeof(GaugeLayerData));
  TextLayer* data_layer = new_large_text_layer();
  GRect data_layer_bounds = {
      .origin = {units_width, 0},
      .size = {bounds.size.w - (2*units_width), bounds.size.h}
  };

  layer_set_frame(text_layer_get_layer(data_layer), data_layer_bounds);
  layer_add_child(layer, text_layer_get_layer(data_layer));

  GaugeLayerData* gauge_data = layer_get_data(layer);
  gauge_data->data_layer = data_layer;
  gauge_data->data = current_data[gauge->message_key];

  Layer* label_layer = new_middle_layer(
      0, units_width, bounds.size.h, gauge->type_label);
  layer_add_child(layer, label_layer);

  Layer* units_layer;
  if (gauge->units_layer_create) {
    units_layer = gauge->units_layer_create(
        data_layer_bounds.origin.x + data_layer_bounds.size.w,
        units_width,
        bounds.size.h,
        gauge->units_label);
  } else {
    units_layer = new_bottom_layer(
        data_layer_bounds.origin.x + data_layer_bounds.size.w,
        units_width,
        bounds.size.h,
        gauge->units_label);
  }
  layer_add_child(layer, units_layer);

  layer_set_update_proc(layer, gauge_update_proc);
  return layer;
}

static Layer*
status_layer_create(GRect bounds, Gauge* gauge)
{
  Layer* layer = layer_create_with_data(bounds, sizeof(GaugeLayerData));
  TextLayer* data_layer = new_small_text_layer();
  GRect data_layer_bounds = {
    .origin = {units_width, bounds.size.h / 2 - DISPLAY_SMALL_FONT_SIZE / 2},
    .size = {bounds.size.w - units_width, DISPLAY_SMALL_FONT_SIZE}
  };

  text_layer_set_overflow_mode(data_layer, GTextOverflowModeTrailingEllipsis);
  layer_set_frame(text_layer_get_layer(data_layer), data_layer_bounds);
  layer_add_child(layer, text_layer_get_layer(data_layer));

  GaugeLayerData* gauge_data = layer_get_data(layer);
  gauge_data->data_layer = data_layer;
  gauge_data->data = current_data[gauge->message_key];

  Layer* label_layer = new_middle_layer(
      0, units_width, bounds.size.h, gauge->type_label);
  layer_add_child(layer, label_layer);

  layer_set_update_proc(layer, gauge_update_proc);
  return layer;
}

static Gauge gauges[] = {
  {
    .message_key = KNOTS,
    .units_label = "KN",
    .type_label = "SOG"
  },
  {
    .message_key = BEARING,
    .units_label = "o",
    .type_label = "BR",
    .units_layer_create = new_top_layer
  },
  {
    .message_key = CURRENT_KNOTS,
    .units_label = "KN",
    .type_label = "CS"
  },
  {
    .message_key = CURRENT_BEARING,
    .units_label = "o",
    .type_label = "CBR",
    .units_layer_create = new_top_layer
  },
  {
    .message_key = TIDE_CHANGE,
    .units_label = "MINS",
    .type_label = "TIDE"
  }
};    

static Gauge statuses[] = {
  {
    .message_key = LATITUDE,
    .type_label = "LAT"
  },
  {
    .message_key = LONGITUDE,
    .type_label = "LNG"
  },
  {
    .message_key = CURRENTS_STATION_NAME,
    .type_label = "CUR"
  },
  {
    .message_key = TIDES_STATION_NAME,
    .type_label = "TIDE"
  },
};

static void gauge_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  int timestamp_height = 0;
  uint32_t gauge_height =
    (bounds.size.h - timestamp_height) / NELEM(gauges);

  /* Gauge layer. */
  gauge_layer = layer_create(bounds);
  for (uint32_t gauge_index = 0;
       gauge_index < NELEM(gauges);
       gauge_index++)
  {
    gauges[gauge_index].layer = gauge_layer_create(
      (GRect) {
        .origin = {0, gauge_index * gauge_height},
        .size = {bounds.size.w, gauge_height}
      },
      &gauges[gauge_index]);
    layer_add_child(gauge_layer, gauges[gauge_index].layer);
  }
  layer_add_child(window_layer, gauge_layer);

  /* Status layer. */
  status_layer = layer_create(bounds);
  for (uint32_t status_index = 0;
       status_index < NELEM(statuses);
       status_index++)
  {
    statuses[status_index].layer = status_layer_create(
      (GRect) {
        .origin = {0, status_index * gauge_height},
        .size = {bounds.size.w, gauge_height}
      },
      &statuses[status_index]);
    layer_add_child(status_layer, statuses[status_index].layer);
  }
  layer_add_child(window_layer, status_layer);
  layer_set_hidden(status_layer, true);

  /* Debug layer. */
  debug_layer = text_layer_create((GRect) {
      .origin = {bounds.size.w - 14, 0},
      .size = {14, 14}});
  text_layer_set_text(debug_layer, debug_values[0]);
  text_layer_set_font(debug_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(debug_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(debug_layer));

#if 0
  /* Invert layer. */
  if (invert_windows) {
    layer_add_child(
      window_layer, inverter_layer_get_layer(inverter_layer_create(bounds)));
  }
#endif
  return;
}

static void
gauge_window_unload(Window *window)
{
}

static void
mark_gauges_dirty(const uint32_t message_key)
{
  for (unsigned gauge_index = 0;
       gauge_index < NELEM(gauges);
       gauge_index++)
  {
    if (gauges[gauge_index].message_key == message_key &&
        gauges[gauge_index].layer)
    {
      layer_mark_dirty(gauges[gauge_index].layer);
    }
  }
  for (unsigned status_index = 0;
       status_index < NELEM(statuses);
       status_index++)
  {
    if (statuses[status_index].message_key == message_key &&
        statuses[status_index].layer)
    {
      layer_mark_dirty(statuses[status_index].layer);
    }
  }
}

static void
javascript_interval(void *data)
{
  /* Poke the JS. This is an alternative to setInterval():
   * http://developer.getpebble.com/blog/2013/12/20/Pebble-Javascript-Tips-and-Tricks/
   */
  app_message_outbox_send();
  app_timer_register(
      1000 * javascript_interval_seconds, javascript_interval, NULL);
}

static void config_provider(Window *window);

static void
select_click_handler(ClickRecognizerRef recognizer, void *context)
{
  layer_set_hidden(gauge_layer, true);
  layer_set_hidden(status_layer, false);
}

static void
select_release_handler(ClickRecognizerRef recognizer, void *context)
{
  layer_set_hidden(gauge_layer, false);
  layer_set_hidden(status_layer, true);
}

static void
config_provider(Window *window)
{
  window_raw_click_subscribe(
    BUTTON_ID_SELECT, select_click_handler, select_release_handler, NULL);
}

static void
sync_changed_handler(const uint32_t key,
                     const Tuple* new_tuple,
                     const Tuple* old_tuple,
                     void* context)
{
  debug_count++;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "sync_changed_handler(%lu): %lu, %s",
          debug_count,
          key,
          new_tuple->value->cstring);

  uint32_t length = MIN(sizeof(current_data[key]), new_tuple->length);
  memcpy(current_data[key], new_tuple->value->cstring, length - 1);
  current_data[key][length - 1] = 0;

  mark_gauges_dirty(key);

  if (debug_layer) {
    text_layer_set_text(debug_layer,
                        debug_values[debug_count % NELEM(debug_values)]);
  }
}

static void
sync_error_handler(DictionaryResult dict_error,
                   AppMessageResult app_message_error,
                   void* context)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "sync_error_handler: %u, %u",
          dict_error,
          app_message_error);
}

static Window*
init(void)
{
  Window* window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = gauge_window_load,
    .unload = gauge_window_unload,
  });

  if (!screenshot_mode) {
    static AppSync app_sync;
    static uint8_t app_sync_buffer[512];

    app_message_open(
      app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

    Tuplet initial_values[] = {
      TupletCString(KNOTS, INITIAL_GAUGE_VALUE),
      TupletCString(BEARING, INITIAL_GAUGE_VALUE),
      TupletCString(CURRENT_KNOTS, INITIAL_GAUGE_VALUE),
      TupletCString(CURRENT_BEARING, INITIAL_GAUGE_VALUE),
      TupletCString(TIDE_CHANGE, INITIAL_GAUGE_VALUE),
      TupletCString(LATITUDE, INITIAL_GAUGE_VALUE),
      TupletCString(LONGITUDE, INITIAL_GAUGE_VALUE),
      TupletCString(CURRENTS_STATION_NAME, INITIAL_GAUGE_VALUE),
      TupletCString(TIDES_STATION_NAME, INITIAL_GAUGE_VALUE),
    };

    app_sync_init(&app_sync,
                  app_sync_buffer,
                  sizeof(app_sync_buffer),
                  initial_values, ARRAY_LENGTH(initial_values),
                  sync_changed_handler,
                  sync_error_handler,
                  NULL);
  } else {
    strcpy(current_data[KNOTS], "6.25");
    strcpy(current_data[BEARING], "110");
    strcpy(current_data[CURRENT_KNOTS], "1.87");
    strcpy(current_data[CURRENT_BEARING], "233");
    strcpy(current_data[TIDE_CHANGE], "H46");
    strcpy(current_data[LATITUDE], "37.77624");
    strcpy(current_data[LONGITUDE], "-122.24741");
    strcpy(current_data[CURRENTS_STATION_NAME], "Oakland Harbor");
    strcpy(current_data[TIDES_STATION_NAME], "Oakland Harbor");
  }

  window_set_click_config_provider(
    window, (ClickConfigProvider) config_provider);

  javascript_interval(NULL);
  window_stack_push(window, true);
  return window;
}

int
main(void)
{
  Window* window = init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing");
  app_event_loop();
  window_destroy(window);
}
