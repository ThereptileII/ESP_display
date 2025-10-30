// touch_integration.cpp — LVGL v8 ONLY
#include "touch_integration.h"
#include "lvgl_v8_guard.h"  // <— add this line
#include "lvgl_indev_compat.h"
#include "config.h"
#include <Arduino.h>

#if __has_include("pins_config.h")
  #include "pins_config.h"
#endif
#ifndef TP_I2C_SDA
  #define TP_I2C_SDA  10
#endif
#ifndef TP_I2C_SCL
  #define TP_I2C_SCL  11
#endif
#ifndef TP_RST
  #define TP_RST      12
#endif
#ifndef TP_INT
  #define TP_INT      13
#endif

#include "gsl3680_touch.h"

static gsl3680_touch *s_touch = nullptr;
static lv_indev_t    *s_indev = nullptr;
static bool           s_draw_dot = false;
static lv_obj_t      *s_dot = nullptr;

static void indev_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  (void)drv;
  static uint16_t last_x = 0, last_y = 0;
  bool pressed = false;
  uint16_t rx=0, ry=0;
  if (s_touch) pressed = s_touch->getTouch(&rx, &ry);

  uint16_t x = rx, y = ry;
#if TOUCH_SWAP_XY
  uint16_t t = x; x = y; y = t;
#endif

  uint16_t w = lv_disp_get_hor_res(NULL);
  uint16_t h = lv_disp_get_ver_res(NULL);

#if TOUCH_INVERT_X
  x = (w > 0) ? (w - 1 - x) : x;
#endif
#if TOUCH_INVERT_Y
  y = (h > 0) ? (h - 1 - y) : y;
#endif

  if (pressed) {
    last_x = x; last_y = y;
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
    data->point.x = last_x;
    data->point.y = last_y;
  }

  if (s_draw_dot) {
    if (!s_dot) {
      s_dot = lv_obj_create(lv_layer_top());
      lv_obj_set_size(s_dot, 10, 10);
      lv_obj_set_style_bg_color(s_dot, lv_color_hex(0xFF0000), 0);
      lv_obj_set_style_radius(s_dot, LV_RADIUS_CIRCLE, 0);
      lv_obj_add_flag(s_dot, LV_OBJ_FLAG_IGNORE_LAYOUT);
    }
    if (pressed) {
      lv_obj_move_foreground(s_dot);
      lv_obj_set_pos(s_dot, data->point.x-5, data->point.y-5);
      lv_obj_set_style_opa(s_dot, LV_OPA_COVER, 0);
    } else {
      lv_obj_set_style_opa(s_dot, LV_OPA_0, 0);
    }
  }
}

lv_indev_t* touch_init_and_register(void) {
  if (s_indev) return s_indev;

  s_touch = new gsl3680_touch(TP_I2C_SDA, TP_I2C_SCL, TP_RST, TP_INT);
  s_touch->begin();
  s_touch->set_rotation(1); // base rotation; mapping fixes in config.h

  static lv_indev_drv_t drv;
  lv_indev_drv_init(&drv);
  drv.type = LV_INDEV_TYPE_POINTER;
  drv.read_cb = indev_read_cb;
#if LV_VERSION_CHECK(8, 3, 0)
  drv.read_period = 15;
#endif
  s_indev = lv_indev_drv_register(&drv);

  lvgl_set_indev_read_period(s_indev, 15);

  Serial.println("[touch] GSL3680 initialized and LVGL v8 indev registered.");
  return s_indev;
}

void touch_debug_overlay_enable(bool enable) { s_draw_dot = enable; }
