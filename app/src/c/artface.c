#include <pebble.h>

#define W 120
#define H 62
#define X (PBL_DISPLAY_WIDTH - W) / 2 + 1
#define Y (PBL_DISPLAY_HEIGHT / 3) - (H / 3)
#define Y_DATE 18

#define SHAPE_COUNT 30
#define TYPE_COUNT 15

static Window *s_window;
static Layer *s_layer;
static TextLayer *s_date_layer;
static TextLayer *s_time_layer;

typedef struct {
  short type;
  short x;
  short y;
  short w;
  short h;
  short r;
  GColor8 color;
} Shape;

static Shape shapes[SHAPE_COUNT] = {[0 ... (SHAPE_COUNT - 1)] = {.type = 0}};

static void render(struct Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_width(ctx, 2);
  for (int i = 0; i < SHAPE_COUNT; i++) {
    Shape shape = shapes[i];
    graphics_context_set_stroke_color(ctx, shape.color);
    graphics_context_set_fill_color(ctx, shape.color);
    switch (shape.type) {
      case 1 ... 3:
        graphics_draw_line(ctx, GPoint(shape.x, shape.y), GPoint(shape.w, shape.h));
        break;
      case 4 ... 6:
        graphics_draw_rect(ctx, GRect(shape.x, shape.y, shape.x + shape.w, shape.y + shape.h));
        break;
      case 7 ... 8:
        graphics_draw_circle(ctx, GPoint(shape.x, shape.y), shape.w);
        break;
      case 9 ... 11:
        graphics_draw_round_rect(ctx, GRect(shape.x, shape.y, shape.x + shape.w, shape.y + shape.h), shape.r);
        break;
      case 12:
        graphics_fill_rect(ctx, GRect(shape.x, shape.y, shape.x + shape.w, shape.y + shape.h), 0, GCornerNone);
        break;
      case 13:
        graphics_fill_circle(ctx, GPoint(shape.x, shape.y), shape.w);
        break;
      case 14:
        graphics_fill_rect(ctx, GRect(shape.x, shape.y, shape.x + shape.w, shape.y + shape.h), shape.r, GCornersAll);
        break;
    }
  }
}

static void handle_time(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & DAY_UNIT) {
    static char date_buffer[9];
    strftime(date_buffer, 9, "%x", tick_time);
    text_layer_set_text(s_date_layer, date_buffer);
  }
  if (units_changed & MINUTE_UNIT) {
    static char time_buffer[6];
    strftime(time_buffer, 6, clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    text_layer_set_text(s_time_layer, time_buffer);
  }
  if (units_changed & HOUR_UNIT) {
    Shape shape = {
      .type = rand() % TYPE_COUNT,
      .x = rand() % PBL_DISPLAY_WIDTH,
      .y = rand() % PBL_DISPLAY_HEIGHT,
      .w = rand() % (PBL_DISPLAY_WIDTH / 2),
      .h = rand() % (PBL_DISPLAY_HEIGHT / 2),
      .r = rand() % 8 + 1,
      .color = GColorFromRGB(rand() % 256, rand() % 256, rand() % 256),
    };
    shapes[rand() % SHAPE_COUNT] = shape;
  }
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  s_layer = layer_create(GRect(0, 0, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT));
  layer_set_update_proc(s_layer, render);
  layer_add_child(window_layer, s_layer);

  s_date_layer = text_layer_create(GRect(X, Y, W, Y_DATE));
  text_layer_set_background_color(s_date_layer, GColorWhite);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  s_time_layer = text_layer_create(GRect(X, Y + Y_DATE, W, H - Y_DATE));
  text_layer_set_background_color(s_time_layer, GColorWhite);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_layer);
}

static void prv_init(void) {
  if (persist_exists(1)) {
    persist_read_data(1, &shapes, sizeof(shapes));
  }

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, false);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_time);
  time_t now = time(NULL);
  handle_time(localtime(&now), MINUTE_UNIT | DAY_UNIT);
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();

  window_destroy(s_window);
  persist_write_data(1, &shapes, sizeof(shapes));
}

int main(void) {
  setlocale(LC_ALL, "");
  prv_init();
  app_event_loop();
  prv_deinit();
}
