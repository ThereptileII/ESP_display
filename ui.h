#pragma once
#include <lvgl.h>
#include <Arduino.h>
#include "config.h"
lv_obj_t* ui_build();
void ui_set_night_mode(bool enabled);
void ui_update_rpm(uint16_t rpm);
void ui_update_power_kw(float kw);
void ui_update_batt_v(float v);
void ui_update_wind(float speed_ms, float angle_rad);
void ui_open_battery_detail();
void ui_close_battery_detail();
void ui_ap_open();
void ui_ap_close();
void ui_next_page();
void ui_prev_page();
void ui_tick();
