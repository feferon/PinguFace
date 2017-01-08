#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_day_label, *s_num_label;
static char s_num_buffer[6];
static TextLayer *s_battery_layer;
static TextLayer *s_connection_layer;
static GBitmap *s_bitmap;
static BitmapLayer *s_layer;
static char *skdays[8];
static int dow = 1;

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "nabijam 100%";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "nabijam %d%%", charge_state.charge_percent);
    text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
    text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  }
  text_layer_set_text(s_battery_layer, battery_text);
}

static void handle_minute_tick(struct tm* tick_time, TimeUnits units_changed) {
  static char s_time_text[] = "00:00";
   
  dow = tick_time->tm_wday;
    
  if( (units_changed & MINUTE_UNIT) != 0 ) {
        /* Minutes changed */
      strftime(s_time_text, sizeof(s_time_text), "%T", tick_time);
      text_layer_set_text(s_time_layer, s_time_text);
  }

  if( (units_changed & DAY_UNIT) != 0 ) {
      /* Days changed */
     strftime(s_num_buffer, sizeof(s_num_buffer), "%d.%m", tick_time);
     text_layer_set_text(s_num_label, s_num_buffer);
     text_layer_set_text(s_day_label, skdays[dow]);
  }
}

static void handle_bluetooth(bool connected) {
  text_layer_set_text(s_connection_layer, connected ? "pripojene" : "odpojene");
  if(!connected) {
    vibes_double_pulse();
  }
}

static void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
}


static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP,  up_single_click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ingaemma_c);

  GPoint center = grect_center_point(&bounds);
  GSize image_size = gbitmap_get_bounds(s_bitmap).size;
  GRect image_frame = GRect(center.x, center.y, image_size.w, image_size.h);
  image_frame.origin.x -= image_size.w / 2;
  image_frame.origin.y -= image_size.h / 2;
  s_layer = bitmap_layer_create(image_frame);
  bitmap_layer_set_bitmap(s_layer, s_bitmap);
  bitmap_layer_set_compositing_mode(s_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_layer));
  
  skdays[0] = "Nedela";
  skdays[1] = "Pondelok";
  skdays[2] = "Utorok";
  skdays[3] = "Streda";
  skdays[4] = "Stvrtok";
  skdays[5] = "Piatok";
  skdays[6] = "Sobota";
  skdays[7] = "Nedela";

  s_time_layer = text_layer_create(GRect(0, 0, bounds.size.w, 34));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);

  s_day_label = text_layer_create(GRect(0, 0, bounds.size.w, 34));
  text_layer_set_background_color(s_day_label, GColorClear);
  text_layer_set_text_color(s_day_label, GColorWhite);
  text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_day_label, GTextAlignmentRight);
  
  s_num_label = text_layer_create(GRect(0, 15, bounds.size.w, 34));
  text_layer_set_background_color(s_num_label, GColorClear);
  text_layer_set_text_color(s_num_label, GColorWhite);
  text_layer_set_font(s_num_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_num_label, GTextAlignmentRight);
  
  s_connection_layer = text_layer_create(GRect(0, 145, bounds.size.w, 34));
  text_layer_set_text_color(s_connection_layer, GColorWhite);
  text_layer_set_background_color(s_connection_layer, GColorClear);
  text_layer_set_font(s_connection_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_connection_layer, GTextAlignmentRight);
  handle_bluetooth(connection_service_peek_pebble_app_connection());

  s_battery_layer = text_layer_create(GRect(0, 145, bounds.size.w, 34));
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentLeft);

  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  
  dow = current_time->tm_wday;
  
  handle_minute_tick(current_time, DAY_UNIT);
  handle_minute_tick(current_time, MINUTE_UNIT);
  
  tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT, handle_minute_tick);
    
  battery_state_service_subscribe(handle_battery);

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_day_label));
  layer_add_child(window_layer, text_layer_get_layer(s_num_label));
  layer_add_child(window_layer, text_layer_get_layer(s_connection_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

  handle_battery(battery_state_service_peek());
}

static void main_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_connection_layer);
  text_layer_destroy(s_battery_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_background_color(s_main_window, GColorBlack); 
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
