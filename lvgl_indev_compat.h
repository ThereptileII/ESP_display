#pragma once

#include "lvgl_v8_guard.h"

#ifdef __cplusplus
/*
 * Some vendor LVGL forks changed lv_indev_get_read_timer() to take an lv_disp_t
 * instead of lv_indev_t.  Use overload resolution to bridge both signatures at
 * compile time without relying on brittle version checks.
 */
static inline lv_timer_t *lvgl_indev_get_timer(lv_indev_t *indev,
                                               lv_timer_t *(*fn)(lv_indev_t *)) {
  return fn(indev);
}

static inline lv_timer_t *lvgl_indev_get_timer(lv_indev_t *indev,
                                               lv_timer_t *(*fn)(lv_disp_t *)) {
  lv_disp_t *disp = lv_indev_get_disp(indev);
  return disp ? fn(disp) : nullptr;
}

template <typename T>
static inline auto lvgl_indev_drv_set_read_period_if_present(T &drv, uint32_t period_ms)
    -> decltype(drv.read_period = period_ms, void()) {
  drv.read_period = period_ms;
}

static inline void lvgl_indev_drv_set_read_period_if_present(...) {}
#endif  // __cplusplus

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

#ifdef __cplusplus
  lv_timer_t *read_timer = lvgl_indev_get_timer(indev, &lv_indev_get_read_timer);
#else
  lv_timer_t *read_timer = lv_indev_get_read_timer(indev);
#endif
  if (read_timer) {
    lv_timer_set_period(read_timer, period_ms);
  }
}
