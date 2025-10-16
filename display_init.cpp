/*
 * display_init.cpp
 *
 * Implementation of display and touch initialisation using the
 * M5GFX library for the JC8012P4A1 10.1" module.  This code
 * sets up LVGL with double buffering in PSRAM and configures a
 * touch input device.  If your project uses another driver,
 * implement the same interface accordingly.
 */

#include "display_init.h"
#include "config.h"

#include <M5GFX.h>
#include <Arduino.h>

// M5GFX instance
static M5GFX gfx;

// LVGL flush callback
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  int32_t w = area->x2 - area->x1 + 1;
  int32_t h = area->y2 - area->y1 + 1;
  gfx.startWrite();
  gfx.setAddrWindow(area->x1, area->y1, w, h);
  // cast to rgb565
  gfx.pushPixels((lgfx::rgb565_t*)px_map, w * h, true);
  gfx.endWrite();
  lv_display_flush_ready(disp);
}

// LVGL touch read callback
static void lvgl_touch_cb(lv_input_device_t * indev, lv_input_device_data_t * data) {
  uint16_t x, y;
  if (gfx.getTouch(&x, &y)) {
    data->point.x = x;
    data->point.y = y;
    data->state = LV_INDEV_STATE_PR;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

namespace display_init {
  void initDisplayAndTouch() {
    // initialise M5GFX and rotate to landscape
    gfx.begin();
    gfx.setRotation(1);
    gfx.setBrightness(255);
    // fill screen black while initialising
    gfx.fillScreen(TFT_BLACK);
    // LVGL initialisation
    lv_init();
    // allocate two partial buffers in PSRAM
    const size_t buf_size = DISPLAY_WIDTH * 40 /*lines*/ * sizeof(uint16_t);
    uint8_t *buf1 = (uint8_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    uint8_t *buf2 = (uint8_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    lv_display_t *disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // touch input
    lv_input_device_t * indev = lv_input_device_create();
    lv_input_device_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_input_device_set_read_cb(indev, lvgl_touch_cb);
  }
} // namespace display_init
