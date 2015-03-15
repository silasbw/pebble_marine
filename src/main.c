#include <pebble.h>

#define DISPLAY_LARGE_FONT FONT_KEY_BITHAM_30_BLACK
#define DISPLAY_LARGE_FONT_SIZE 30
#define DISPLAY_SMALL_FONT FONT_KEY_GOTHIC_14_BOLD
#define DISPLAY_SMALL_FONT_SIZE 14

#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

enum
{
  KNOTS = 1,
  BEARING = 2,
  CURRENT_KNOTS = 3,
  CURRENT_BEARING = 4,
  TIDE_CHANGE = 5,

  NUMBER_OF_MESSAGE_KEYS
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

static Window* window;
static uint32_t debug_count;
static TextLayer* debug_layer;
static const char* debug_values[] = {"|", "--"};

static char current_data[NUMBER_OF_MESSAGE_KEYS][128];


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


Layer* gauge_layer_create(GRect bounds, Gauge* gauge)
{
  Layer* layer = layer_create_with_data(bounds, sizeof(GaugeLayerData));
  TextLayer* data_layer = new_large_text_layer();
  GRect data_layer_bounds = {
      .origin = {units_width, 0},
      .size = {bounds.size.w - (2*units_width), bounds.size.h}
  };

  text_layer_set_text(data_layer, "---");
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


Gauge gauges[] = {
  {
    .message_key = KNOTS,
    .units_label = "kts",
    .type_label = "sog"
  },
  {
    .message_key = BEARING,
    .units_label = "o",
    .type_label = "br",
    .units_layer_create = new_top_layer
  },
  {
    .message_key = CURRENT_KNOTS,
    .units_label = "kts",
    .type_label = "cs"
  },
  {
    .message_key = CURRENT_BEARING,
    .units_label = "o",
    .type_label = "cbr",
    .units_layer_create = new_top_layer
  },
  {
    .message_key = TIDE_CHANGE,
    .units_label = "mins",
    .type_label = "tide"
  }
};    


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  int timestamp_height = 0;
  uint32_t gauge_height =
    (bounds.size.h - timestamp_height) / NELEM(gauges);

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
    layer_add_child(window_layer, gauges[gauge_index].layer);
  }

  debug_layer = text_layer_create((GRect) {
      .origin = {bounds.size.w - 14, 0},
      .size = {14, 14}});
  text_layer_set_text(debug_layer, debug_values[0]);
  text_layer_set_font(debug_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(debug_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(debug_layer));
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
}


static void
in_received_handler(DictionaryIterator* received, void* context)
{
  for (uint32_t message_key = 0;
       message_key < NUMBER_OF_MESSAGE_KEYS;
       message_key++)
  {
    Tuple* tuple = dict_find(received, message_key);
    if (tuple && strcmp(current_data[message_key], tuple->value->cstring)) {
      memcpy(current_data[message_key], tuple->value->cstring, tuple->length);
      mark_gauges_dirty(message_key);
    }
  }

  debug_count++;
  text_layer_set_text(debug_layer,
                      debug_values[debug_count % NELEM(debug_values)]);
}


static void
in_dropped_handler(AppMessageResult reason, void *context)
{
}


static void
out_sent_handler(DictionaryIterator *sent, void *context)
{
}


static void
out_failed_handler(DictionaryIterator *failed,
                   AppMessageResult reason,
                   void *context)
{
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


static void window_unload(Window *window)
{
  for (unsigned gauge_index = 0;
       gauge_index < NELEM(gauges);
       gauge_index++)
    if (gauges[gauge_index].layer)
      layer_destroy(gauges[gauge_index].layer);
}


static void init(void)
{
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  app_message_open(app_message_inbox_size_maximum(),
                   app_message_outbox_size_maximum());

  javascript_interval(NULL);
  window_stack_push(window, true);
}


int
main(void)
{
  init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  app_event_loop();
  window_destroy(window);
}