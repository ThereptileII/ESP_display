// main.ino — LVGL v8 ONLY
#include <Arduino.h>
#include <lvgl.h>
#include "lvgl_v8_guard.h"   // <— add this line
#include "config.h"
#include "display_driver.h"
#include "ui.h"
#include "can_bus.h"
#include "sdlog.h"
#include "touch_integration.h"

static lv_disp_t* g_disp = nullptr;
static bool g_sd_ok = false;
static SeriesRuntime s1h, s6h, s24h;

void setup() {
  Serial.begin(115200);
  delay(200);

  // Initializes panel + LVGL (lv_init is called inside)
  g_disp = display_port_init();

  ui_build();
  touch_init_and_register();
  touch_debug_overlay_enable(false);

  g_sd_ok = sdlog_begin();
  if (g_sd_ok) {
    sdlog_open_series("battery_v_1h");
    sdlog_open_series("battery_v_6h");
    sdlog_open_series("battery_v_24h");
  }
  // 1h @3.5s raw, 6h avg@21s, 24h avg@84s
  series_init(s1h,  { "1h",  1024, 3500  });
  series_init(s6h,  { "6h",  1024, 21000 });
  series_init(s24h, { "24h", 1024, 84000 });

  CANBRIDGE_UART.begin(CANBRIDGE_BAUD, SERIAL_8N1, CANBRIDGE_RX, CANBRIDGE_TX);
  canbridge_begin(CANBRIDGE_UART);
}

static float    filt_acc    = 0;
static uint32_t filt_count  = 0;
static uint32_t last_avg_ms = 0;

static void handle_pgn(uint32_t pgn, const uint8_t* d, uint8_t len) {
  if (pgn == 127508 && len >= 5) {                 // Battery Status (Voltage)
    uint16_t mv = (uint16_t)d[2] | ((uint16_t)d[3] << 8);
    float v = mv / 100.0f;
    ui_update_batt_v(v);

    uint32_t now = millis();
    if (series_maybe_store(s1h, now, v)) sdlog_append_csv("battery_v_1h", now, v);

    // Aggregate for 6h/24h series
    filt_acc += v; filt_count++;
    if (now - last_avg_ms >= 21000) {
      float avg = (filt_count ? (filt_acc / filt_count) : v);
      series_maybe_store(s6h, now, avg); sdlog_append_csv("battery_v_6h", now, avg);

      static uint32_t last_24 = 0;
      static float acc24 = 0; static uint32_t cnt24 = 0;
      acc24 += avg; cnt24++;
      if (now - last_24 >= 84000) {
        float avg24 = cnt24 ? (acc24 / cnt24) : avg;
        series_maybe_store(s24h, now, avg24);
        sdlog_append_csv("battery_v_24h", now, avg24);
        last_24 = now; acc24 = 0; cnt24 = 0;
      }
      last_avg_ms = now; filt_acc = 0; filt_count = 0;
    }
  }
  else if (pgn == 130306 && len >= 5) {            // Wind
    uint16_t sp = (uint16_t)d[1] | ((uint16_t)d[2] << 8);
    uint16_t ar = (uint16_t)d[3] | ((uint16_t)d[4] << 8);
    float speed_ms = sp * 0.01f;
    float angle_rad = ar * 0.0001f;
    ui_update_wind(speed_ms, angle_rad);
  }
}

void loop() {
  // LVGL v8 tick + handler
  static uint32_t last = 0;
  uint32_t now = millis();
  uint32_t dt  = now - last;
  if (dt >= 12) {
    last = now;
    lv_tick_inc(dt);      // LVGL 8 API
    lv_timer_handler();   // LVGL 8 API
  } else {
    delay(1);
  }

  // CAN-bridge SLCAN read
  CanFrame f;
  if (canbridge_read(f) && f.valid) {
    uint32_t pgn = n2k_pgn(f.id);
    handle_pgn(pgn, f.data, f.len);
  }

  ui_tick();
}
