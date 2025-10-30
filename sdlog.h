#pragma once
#include <Arduino.h>
struct SeriesConfig { const char* name; uint32_t points; uint32_t interval_ms; };
struct SeriesRuntime { SeriesConfig cfg; float* values; uint32_t head; uint32_t last_store; };
bool sdlog_begin();
bool sdlog_open_series(const char* measurement);
void sdlog_append_csv(const char* measurement, uint32_t ms, float value);
void series_init(SeriesRuntime& s, const SeriesConfig& cfg);
bool series_maybe_store(SeriesRuntime& s, uint32_t now_ms, float value);
