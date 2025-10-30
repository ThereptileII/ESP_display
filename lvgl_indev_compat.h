#pragma once

#include "lvgl_v8_guard.h"

static inline void lvgl_set_indev_read_period(lv_indev_t *indev, uint32_t period_ms) {
#if LV_VERSION_CHECK(8, 3, 0)
  if (indev) {
    lv_indev_set_read_period(indev, period_ms);
  }
#else
  if (!indev) {
    return;
  }
  lv_timer_t *read_timer = lv_indev_get_read_timer(indev);
  if (read_timer) {
    lv_timer_set_period(read_timer, period_ms);
  }
#endif
}
