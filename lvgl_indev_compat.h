#pragma once

#include "lvgl_v8_guard.h"

static inline void lvgl_set_indev_read_period(lv_indev_t *indev, uint32_t period_ms) {
  if (!indev) {
    return;
  }

#if LV_VERSION_CHECK(8, 3, 0)
#if !defined(LVGL_USE_INDEV_SET_READ_PERIOD)
#define LVGL_USE_INDEV_SET_READ_PERIOD 0
#endif
#if LVGL_USE_INDEV_SET_READ_PERIOD
  /*
   * Allow advanced projects to opt-in to the 8.3+ helper. By default we keep
   * using the legacy timer path because some vendor forks misreport their
   * version yet omit lv_indev_set_read_period() from the public headers.
   */
  lv_indev_set_read_period(indev, period_ms);
  return;
#endif
#endif

  lv_timer_t *read_timer = lv_indev_get_read_timer(indev);
  if (read_timer) {
    lv_timer_set_period(read_timer, period_ms);
  }
}
