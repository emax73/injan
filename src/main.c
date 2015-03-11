#include <pebble.h>

Window *root_window;
Layer *root_layer;
BitmapLayer *face_layer;
GBitmap *face_bitmap, *battery_bitmap, *bt_on_bitmap, *bt_off_bitmap;
Layer *hands_layer;
TextLayer *time_layer, *date_layer;
GFont *time_font, *date_font, *battery_font;
BitmapLayer *battery_layer, *bt_layer;
TextLayer *battery_text_layer;

static const uint32_t const lost_vibe[] = { 1000, 1000, 1000, 1000, 1000 };
VibePattern lost_vibe_pat = {
  .durations = lost_vibe,
  .num_segments = ARRAY_LENGTH(lost_vibe),
};

#define WIDTH 144
#define HEIGHT 168  

#define HANDS_X 72
#define HANDS_Y 126  
  
#define HOUR_W 22
#define HOUR_H 5
#define MINUTE_W 35
#define MINUTE_H 5
#define SECOND_W 34
#define CENTER_R 5

#define BATTERY_W 40
#define BATTERY_H 19  

#define BT_W 25
#define BT_H 24  
  
#define ANGLE_K (TRIG_MAX_ANGLE / 360)
  
int time_minutes = 0, time_seconds = 0;  
int time_minutes0 = -1, time_seconds0 = -1;
int days0 = -1, days = 0;

char time_text[] = "                  ", date_text[] = "                   ", bat_text[] = "     ";
  
GPath *hour_hand;
static const GPathInfo  HOUR_HAND_PATH = {
  .num_points = 4,
  .points = (GPoint []) {{-HOUR_H, 0}, {-2, -HOUR_W}, {2, -HOUR_W}, {HOUR_H, 0}}
};

GPath *minute_hand;
static const GPathInfo  MINUTE_HAND_PATH = {
  .num_points = 4,
  .points = (GPoint []) {{-MINUTE_H, 0}, {-2, -MINUTE_W}, {2, -MINUTE_W}, {MINUTE_H, 0}}
};

GPath *second_hand;
static const GPathInfo  SECOND_HAND_PATH = {
  .num_points = 2,
  .points = (GPoint []) {{0, 0}, {0, -SECOND_W}}
};

char * weekdays[] = {
  "Вос",
  "Пон",
  "Втр",
  "Срд",
  "Чет",
  "Пят",
  "Суб"
}; 

char * monts[] = {
  "Янв",
  "Фев",
  "Мар",
  "Апр",
  "Мая",
  "Июн",
  "Июл",
  "Авг",
  "Сен",
  "Окт",
  "Ноя",
  "Дек"
};
  
void paths_create() {
  hour_hand = gpath_create(&HOUR_HAND_PATH);
  gpath_move_to(hour_hand, GPoint(HANDS_X, HANDS_Y));

  minute_hand = gpath_create(&MINUTE_HAND_PATH);
  gpath_move_to(minute_hand, GPoint(HANDS_X, HANDS_Y));

  second_hand = gpath_create(&SECOND_HAND_PATH);
  gpath_move_to(second_hand, GPoint(HANDS_X, HANDS_Y));
}

void draw_hands(GContext *ctx) {
  int s = ANGLE_K * time_seconds * 6;

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorWhite);
 
  gpath_draw_filled(ctx, minute_hand);
  gpath_draw_outline(ctx, minute_hand);
  
  gpath_draw_filled(ctx, hour_hand);
  gpath_draw_outline(ctx, hour_hand);

  gpath_rotate_to(second_hand, s);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_outline(ctx, second_hand);
  
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(HANDS_X, HANDS_Y), CENTER_R);
  graphics_draw_circle(ctx, GPoint(HANDS_X, HANDS_Y), CENTER_R);
}

static void hands_layer_update_proc(struct Layer *layer, GContext *ctx) {
    draw_hands(ctx);
}

static void seconds_tick_handler(struct tm * tick_time, TimeUnits units_change) {
    if (tick_time == NULL) {
       time_t tt = time(NULL);
       tick_time = localtime(&tt);
    }
    time_minutes = tick_time->tm_hour * 60 + tick_time->tm_min;
    time_seconds = tick_time->tm_sec;
    if (time_minutes != time_minutes0) {
      int h = ANGLE_K * (time_minutes * 30 / 60);
      int m = ANGLE_K * (time_minutes % 60) * 6;

      gpath_rotate_to(hour_hand, h);
      gpath_rotate_to(minute_hand, m);
 
      strftime(time_text, sizeof(time_text), "%H:%M", tick_time);
      text_layer_set_text(time_layer, time_text);
      
      time_minutes0 = time_minutes;
    }
    layer_mark_dirty(hands_layer);
    days = tick_time->tm_yday;
    if (days != days0) {
      snprintf(date_text, sizeof(date_text), "%i %s, %s", tick_time->tm_mday, monts[tick_time->tm_mon], weekdays[tick_time->tm_wday]);
      text_layer_set_text(date_layer, date_text);
      days0 = days;
    }
}

static void battery_state_handler(BatteryChargeState bat) {
    snprintf(bat_text, sizeof(bat_text), "%i%%", bat.charge_percent);
    text_layer_set_text(battery_text_layer, bat_text);
}

static void bt_connection_handler(bool connected) {
  if (connected) {
    bitmap_layer_set_bitmap(bt_layer, bt_on_bitmap);
  } else {
    vibes_enqueue_custom_pattern(lost_vibe_pat);
    bitmap_layer_set_bitmap(bt_layer, bt_off_bitmap);
  }
  
}

void handle_init(void) {
  root_window = window_create();
  window_set_fullscreen(root_window, true);
  root_layer = window_get_root_layer(root_window);

  face_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FACE);
  
  face_layer = bitmap_layer_create(GRect(0,0,WIDTH, HEIGHT));
  bitmap_layer_set_bitmap(face_layer, face_bitmap);

  battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
  
  battery_layer = bitmap_layer_create(GRect(99, 74, BATTERY_W, BATTERY_H));
  bitmap_layer_set_bitmap(battery_layer, battery_bitmap);

  battery_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);

  battery_text_layer = text_layer_create(GRect(0, 0, BATTERY_W, BATTERY_H));
  text_layer_set_font(battery_text_layer, battery_font);
  text_layer_set_text_alignment(battery_text_layer, GTextAlignmentCenter);
  text_layer_set_background_color(battery_text_layer, GColorClear);
  text_layer_set_text_color(battery_text_layer, GColorBlack);

  bt_on_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ON);
  bt_off_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_OFF);

  bt_layer = bitmap_layer_create(GRect(8, 72, BT_W, BT_H));
  
  paths_create();
  
  hands_layer = layer_create(GRect(0,0,WIDTH, HEIGHT));
  layer_set_update_proc(hands_layer, hands_layer_update_proc);
 
  time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_24));
  date_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  
  time_layer = text_layer_create(GRect(24, 18, 75, 28));
  text_layer_set_font(time_layer, time_font);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_background_color(time_layer, GColorBlack);
  text_layer_set_text_color(time_layer, GColorWhite);
  
  date_layer = text_layer_create(GRect(7, 46, 83, 20));
  text_layer_set_font(date_layer, date_font);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  text_layer_set_background_color(date_layer, GColorBlack);
  text_layer_set_text_color(date_layer, GColorWhite);
  
  layer_add_child(root_layer, bitmap_layer_get_layer(face_layer));
  layer_add_child(root_layer, hands_layer);
  layer_add_child(root_layer, text_layer_get_layer(time_layer));
  layer_add_child(root_layer, text_layer_get_layer(date_layer));
  layer_add_child(root_layer, bitmap_layer_get_layer(battery_layer));
  layer_add_child(bitmap_layer_get_layer(battery_layer), text_layer_get_layer(battery_text_layer));
  layer_add_child(root_layer, bitmap_layer_get_layer(bt_layer));

  battery_state_handler(battery_state_service_peek());
  battery_state_service_subscribe(battery_state_handler);
  
  bt_connection_handler(bluetooth_connection_service_peek());
  bluetooth_connection_service_subscribe(bt_connection_handler);
  
  seconds_tick_handler(NULL, SECOND_UNIT);
  tick_timer_service_subscribe(SECOND_UNIT, seconds_tick_handler);

  window_stack_push(root_window, true);
}

void handle_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
 
  gbitmap_destroy(face_bitmap);
  gbitmap_destroy(battery_bitmap);
  gbitmap_destroy(bt_on_bitmap);
  gbitmap_destroy(bt_off_bitmap);
  fonts_unload_custom_font(time_font);

  gpath_destroy(hour_hand);
  gpath_destroy(minute_hand);
  gpath_destroy(second_hand);
  
  bitmap_layer_destroy(face_layer);
  bitmap_layer_destroy(battery_layer);
  bitmap_layer_destroy(bt_layer);
  layer_destroy(hands_layer);
  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);

  window_destroy(root_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
