#pragma once
#include <lvgl.h>
#ifdef __cplusplus
extern "C" {
#endif
lv_disp_t* display_port_init(void);
int display_get_stripe_lines(void);
int display_get_native_w(void);
int display_get_native_h(void);
#ifdef __cplusplus
}
#endif
