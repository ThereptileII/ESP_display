#include <Arduino.h>
#include <lvgl.h>
#include "display_driver.h"
#include "config.h"

#include "esp_lcd_types.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_system.h"

#include "esp_lcd_jd9365.h"
#include "jd9365_lcd.h"

extern esp_lcd_panel_handle_t panel_handle;

static const int LCD_RST_PIN = 27;
static jd9365_lcd lcd(LCD_RST_PIN);

static constexpr int NATIVE_W = 800;
static constexpr int NATIVE_H = 1280;

static int STRIPE_LINES = 40;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t* lv_buf1 = nullptr;
static lv_color_t* lv_buf2 = nullptr;
static lv_color_t* bounce  = nullptr;
static size_t      bounce_bytes = 0;

static esp_err_t draw_bitmap_retry(int x1, int y1, int x2, int y2, const void* data) {
  esp_err_t err;
  do {
    err = esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2, y2, data);
    if (err == ESP_ERR_INVALID_STATE) vTaskDelay(pdMS_TO_TICKS(1));
  } while (err == ESP_ERR_INVALID_STATE);
  return err;
}

static void my_disp_flush(lv_disp_drv_t* disp, const lv_area_t* a, lv_color_t* px) {
  int32_t w = a->x2 - a->x1 + 1;  if (w > NATIVE_W) w = NATIVE_W;
  int32_t h = a->y2 - a->y1 + 1;
  size_t need = (size_t)w * h * sizeof(lv_color_t);

  if (need > bounce_bytes) {
    int rows_fit = (int)(bounce_bytes / (w * sizeof(lv_color_t)));
    if (rows_fit < 1) rows_fit = 1;
    int y = a->y1;
    const lv_color_t* src = px;
    int remain = h;
    while (remain > 0) {
      int rows = (remain < rows_fit) ? remain : rows_fit;
      size_t chunk = (size_t)w * rows * sizeof(lv_color_t);
      memcpy(bounce, src, chunk);
      draw_bitmap_retry(a->x1, y, a->x1 + w, y + rows, (const void*)bounce);
      src += w * rows; y += rows; remain -= rows;
    }
    lv_disp_flush_ready(disp);
    return;
  }

  memcpy(bounce, px, need);
  draw_bitmap_retry(a->x1, a->y1, a->x1 + w, a->y1 + h, (const void*)bounce);
  lv_disp_flush_ready(disp);
}

lv_disp_t* display_port_init(void) {
  lcd.begin();
  lv_init();

  auto allocDMA = [&](size_t sz)->lv_color_t* {
    return (lv_color_t*)heap_caps_aligned_alloc(64, sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
  };

  size_t stripe_pixels = (size_t)NATIVE_W * STRIPE_LINES;
  size_t lv_bytes      = stripe_pixels * sizeof(lv_color_t);
  lv_buf1 = allocDMA(lv_bytes);
  lv_buf2 = allocDMA(lv_bytes);

  if (!lv_buf1 || !lv_buf2) {
    if (lv_buf1) heap_caps_free(lv_buf1);
    if (lv_buf2) heap_caps_free(lv_buf2);
    STRIPE_LINES = 24;
    size_t stripe_pixels2 = (size_t)NATIVE_W * STRIPE_LINES;
    size_t lv_bytes2      = stripe_pixels2 * sizeof(lv_color_t);
    lv_buf1 = allocDMA(lv_bytes2);
    lv_buf2 = allocDMA(lv_bytes2);
    if (!lv_buf1 || !lv_buf2) {
      Serial.println("LVGL INTERNAL buffer alloc failed; reduce STRIPE_LINES further.");
      while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  bounce_bytes = (size_t)NATIVE_W * STRIPE_LINES * sizeof(lv_color_t);
  bounce = allocDMA(bounce_bytes);
  if (!bounce) {
    Serial.println("Bounce buffer alloc failed; reduce STRIPE_LINES.");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
  }

  lv_disp_draw_buf_init(&draw_buf, lv_buf1, lv_buf2, (size_t)NATIVE_W * STRIPE_LINES);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res  = NATIVE_W;
  disp_drv.ver_res  = NATIVE_H;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.flush_cb = my_disp_flush;
#if ORIENTATION_MODE == 1
  disp_drv.sw_rotate = 1;
#endif
  lv_disp_t* disp = lv_disp_drv_register(&disp_drv);
#if ORIENTATION_MODE == 1
  lv_disp_set_rotation(disp, LV_DISP_ROT_90);
#endif
  return disp;
}

int display_get_stripe_lines(void) { return STRIPE_LINES; }
int display_get_native_w(void){ return NATIVE_W; }
int display_get_native_h(void){ return NATIVE_H; }
