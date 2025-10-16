/*
 * display_init.h
 *
 * Initialises the display and touch drivers for the JC8012P4A1 module.
 * This implementation uses the M5GFX library (LovyanGFX) to drive
 * the JD9365 panel via MIPI DSI and allocate LVGL buffers.  If you
 * use a different driver or vendor library, implement the same
 * interface here.
 */

#pragma once

#include <lvgl.h>

namespace display_init {
  // Perform display, LVGL and touch initialisation.  This must be
  // called before building the UI.
  void initDisplayAndTouch();
}
