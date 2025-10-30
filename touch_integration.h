#pragma once
#include <lvgl.h>
#ifdef __cplusplus
extern "C" {
#endif
lv_indev_t* touch_init_and_register(void);
void touch_debug_overlay_enable(bool enable);
#ifdef __cplusplus
}
#endif
