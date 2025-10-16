/*
 * logging.cpp
 *
 * Implements sampling, SD logging and ring buffer management for
 * battery voltage and true wind speed.  At compile time the buffers
 * allocate SERIES_LENGTH elements (config.h).  When called from
 * logging::tick() the current battery voltage (g_Batt_V) and true
 * wind speed (g_TWS_ms) are recorded and aggregated for 6h and
 * 24h series.  Data are appended to CSV files on the SD card.
 */

#include "logging.h"
#include "config.h"
#include "state.h"

#include <Arduino.h>
#include <SD.h>
#if defined(USE_SD_MMC) && USE_SD_MMC
#include <SD_MMC.h>
#endif

namespace logging {

  // Buffers for battery voltage
  static float bat1h[SERIES_LENGTH];
  static float bat6h[SERIES_LENGTH];
  static float bat24h[SERIES_LENGTH];
  static uint16_t bat_idx1h = 0;
  static uint16_t bat_idx6h = 0;
  static uint16_t bat_idx24h = 0;
  static bool bat_filled1h = false;
  static bool bat_filled6h = false;
  static bool bat_filled24h = false;
  static uint8_t bat_acc6_cnt = 0; static double bat_acc6_sum = 0.0;
  static uint8_t bat_acc24_cnt = 0; static double bat_acc24_sum = 0.0;

  // Buffers for true wind speed
  static float wind1h[SERIES_LENGTH];
  static float wind6h[SERIES_LENGTH];
  static float wind24h[SERIES_LENGTH];
  static uint16_t wind_idx1h = 0;
  static uint16_t wind_idx6h = 0;
  static uint16_t wind_idx24h = 0;
  static bool wind_filled1h = false;
  static bool wind_filled6h = false;
  static bool wind_filled24h = false;
  static uint8_t wind_acc6_cnt = 0; static double wind_acc6_sum = 0.0;
  static uint8_t wind_acc24_cnt = 0; static double wind_acc24_sum = 0.0;

  static uint32_t lastSampleMs = 0;

  // Helper: get reference to FS (SD or SD_MMC)
  static fs::FS &getFS() {
#if defined(USE_SD_MMC) && USE_SD_MMC
    return SD_MMC;
#else
    return SD;
#endif
  }

  // Create log file if missing
  static void ensureFile(const char *path, const char *header) {
    fs::FS &fs = getFS();
    if (!fs.exists(path)) {
      File f = fs.open(path, FILE_WRITE);
      if (f) {
        f.println(header);
        f.close();
      }
    }
  }

  void init() {
#if defined(USE_SD_MMC) && USE_SD_MMC
    SD_MMC.begin();
#else
    SD.begin(SD_CS_PIN);
#endif
    ensureFile(LOG_BATTERY_PATH, "ts_ms,voltage");
    ensureFile(LOG_WIND_PATH,    "ts_ms,aws_ms,tws_ms,awa_deg,twa_deg");
  }

  // Write a row to battery log
  static void logBattery(uint32_t ts, float v) {
    fs::FS &fs = getFS();
    File f = fs.open(LOG_BATTERY_PATH, FILE_APPEND);
    if (f) {
      f.printf("%lu,%.3f\n", (unsigned long)ts, v);
      f.close();
    }
  }

  // Write a row to wind log
  static void logWind(uint32_t ts, float aws, float tws, float awa, float twa) {
    fs::FS &fs = getFS();
    File f = fs.open(LOG_WIND_PATH, FILE_APPEND);
    if (f) {
      f.printf("%lu,%.3f,%.3f,%.1f,%.1f\n", (unsigned long)ts, aws, tws, awa, twa);
      f.close();
    }
  }

  // Push new battery sample into buffers and aggregators
  static void pushBattery(float v) {
    bat1h[bat_idx1h] = v;
    bat_idx1h = (bat_idx1h + 1) % SERIES_LENGTH;
    if (bat_idx1h == 0) bat_filled1h = true;
    // accumulate for 6h
    bat_acc6_sum += v;
    bat_acc6_cnt++;
    if (bat_acc6_cnt >= 6) {
      float avg = bat_acc6_sum / bat_acc6_cnt;
      bat6h[bat_idx6h] = avg;
      bat_idx6h = (bat_idx6h + 1) % SERIES_LENGTH;
      if (bat_idx6h == 0) bat_filled6h = true;
      bat_acc6_cnt = 0;
      bat_acc6_sum = 0.0;
      // accumulate for 24h (from 6h points)
      bat_acc24_sum += avg;
      bat_acc24_cnt++;
      if (bat_acc24_cnt >= 4) {
        float avg24 = bat_acc24_sum / bat_acc24_cnt;
        bat24h[bat_idx24h] = avg24;
        bat_idx24h = (bat_idx24h + 1) % SERIES_LENGTH;
        if (bat_idx24h == 0) bat_filled24h = true;
        bat_acc24_cnt = 0;
        bat_acc24_sum = 0.0;
      }
    }
  }

  // Push new wind sample (true wind speed) into buffers and aggregators
  static void pushWind(float v) {
    wind1h[wind_idx1h] = v;
    wind_idx1h = (wind_idx1h + 1) % SERIES_LENGTH;
    if (wind_idx1h == 0) wind_filled1h = true;
    wind_acc6_sum += v;
    wind_acc6_cnt++;
    if (wind_acc6_cnt >= 6) {
      float avg = wind_acc6_sum / wind_acc6_cnt;
      wind6h[wind_idx6h] = avg;
      wind_idx6h = (wind_idx6h + 1) % SERIES_LENGTH;
      if (wind_idx6h == 0) wind_filled6h = true;
      wind_acc6_cnt = 0;
      wind_acc6_sum = 0.0;
      wind_acc24_sum += avg;
      wind_acc24_cnt++;
      if (wind_acc24_cnt >= 4) {
        float avg24 = wind_acc24_sum / wind_acc24_cnt;
        wind24h[wind_idx24h] = avg24;
        wind_idx24h = (wind_idx24h + 1) % SERIES_LENGTH;
        if (wind_idx24h == 0) wind_filled24h = true;
        wind_acc24_cnt = 0;
        wind_acc24_sum = 0.0;
      }
    }
  }

  void tick() {
    uint32_t now = millis();
    if (now - lastSampleMs < SAMPLE_INTERVAL_MS) return;
    lastSampleMs = now;
    // sample current battery voltage and true wind speed
    float battV = g_Batt_V;
    float tws   = g_TWS_ms;
    pushBattery(battV);
    pushWind(tws);
    logBattery(now, battV);
    logWind(now, g_AWS_ms, g_TWS_ms, g_AWA_deg, g_TWA_deg);
    // update session min/max
    if (g_TWS_ms > g_TWS_max) g_TWS_max = g_TWS_ms;
    if (g_TWS_ms < g_TWS_min) g_TWS_min = g_TWS_ms;
  }

  void getBatterySeries(Range range, const float *&buf, bool &filled, uint16_t &index) {
    switch (range) {
      case Range::R1H:  buf = bat1h;  filled = bat_filled1h;  index = bat_idx1h;  break;
      case Range::R6H:  buf = bat6h;  filled = bat_filled6h;  index = bat_idx6h;  break;
      case Range::R24H: buf = bat24h; filled = bat_filled24h; index = bat_idx24h; break;
    }
  }

  void getWindSeries(Range range, const float *&buf, bool &filled, uint16_t &index) {
    switch (range) {
      case Range::R1H:  buf = wind1h;  filled = wind_filled1h;  index = wind_idx1h;  break;
      case Range::R6H:  buf = wind6h;  filled = wind_filled6h;  index = wind_idx6h;  break;
      case Range::R24H: buf = wind24h; filled = wind_filled24h; index = wind_idx24h; break;
    }
  }
} // namespace logging
