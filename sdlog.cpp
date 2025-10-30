#include "sdlog.h"
#include "config.h"
#if USE_SD_MMC
  #include <SD_MMC.h>
#else
  #include <SD.h>
  #include <SPI.h>
#endif
#include "esp_heap_caps.h"

static bool g_sd_ok = false;

bool sdlog_begin() {
#if USE_SD_MMC
  g_sd_ok = SD_MMC.begin("/sdcard", true);
#else
  g_sd_ok = SD.begin();
#endif
  if (!g_sd_ok) Serial.println("[SD] init failed"); else Serial.println("[SD] init ok");
  return g_sd_ok;
}

static String file_for(const char* measurement) {
  String p = "/"; p += measurement; p += ".csv"; return p;
}

bool sdlog_open_series(const char* measurement) {
  if (!g_sd_ok) return false;
#if USE_SD_MMC
  File f = SD_MMC.open(file_for(measurement), FILE_APPEND);
#else
  File f = SD.open(file_for(measurement), FILE_APPEND);
#endif
  if (!f) return false;
  if (f.size() == 0) f.println("ms,value");
  f.close(); return true;
}

void sdlog_append_csv(const char* measurement, uint32_t ms, float value) {
  if (!g_sd_ok) return;
#if USE_SD_MMC
  File f = SD_MMC.open(file_for(measurement), FILE_APPEND);
#else
  File f = SD.open(file_for(measurement), FILE_APPEND);
#endif
  if (!f) return;
  f.print(ms); f.print(','); f.println(value, 3);
  f.close();
}

void series_init(SeriesRuntime& s, const SeriesConfig& cfg) {
  s.cfg = cfg; s.head = 0; s.last_store = 0;
  s.values = (float*)heap_caps_calloc(cfg.points, sizeof(float), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

bool series_maybe_store(SeriesRuntime& s, uint32_t now_ms, float value) {
  if (now_ms - s.last_store >= s.cfg.interval_ms) {
    s.last_store = now_ms;
    if (s.values) { s.values[s.head] = value; s.head = (s.head + 1) % s.cfg.points; }
    return true;
  }
  return false;
}
