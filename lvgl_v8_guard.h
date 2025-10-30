#pragma once
#include <lvgl.h>

// Force LVGL v8 only. Stop immediately if something else is installed.
#ifndef LVGL_VERSION_MAJOR
# error "LVGL_VERSION_MAJOR is not defined. You are not including a standard LVGL. Install LVGL 8.x via Library Manager."
#endif

#if LVGL_VERSION_MAJOR != 8
# error "Wrong LVGL version detected. This project targets LVGL 8.x ONLY. Uninstall LVGL 9 and install LVGL 8.x (e.g., 8.3.11)."
#endif

// Optional: show versions in verbose compile output
#ifndef LVGL_VERSION_MINOR
#define LVGL_VERSION_MINOR 0
#endif
#ifndef LVGL_VERSION_PATCH
#define LVGL_VERSION_PATCH 0
#endif
