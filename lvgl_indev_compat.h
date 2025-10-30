#pragma once

#include "lvgl_v8_guard.h"

static inline void lvgl_set_indev_read_period(lv_indev_t *indev, uint32_t period_ms) {
#if LV_VERSION_CHECK(8, 3, 0)
  if (indev) {
    lv_indev_set_read_period(indev, period_ms);
  }
#else
  (void)indev;
  lv_disp_t *disp = lv_disp_get_default();
  if (!disp) {
    disp = lv_disp_get_next(NULL);
  }
  if (disp) {
    lv_timer_t *read_timer = lv_indev_get_read_timer(disp);
    if (read_timer) {
      lv_timer_set_period(read_timer, period_ms);
    }
  }
#endif
}
