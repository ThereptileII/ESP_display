/*
 * ui.h
 *
 * Declares functions for creating and updating the user interface.
 * Pages are organised into a TabView: Speed, Overview and Wind.  A
 * persistent button toggles night mode.  A slide‑down sheet provides
 * an autopilot controller.  All UI updates read from state.h.
 */

#pragma once

#include <lvgl.h>

namespace ui {
  // Initialise styles, build pages and register callbacks.  Must be
  // called after LVGL and display are initialised.
  void init();

  // Toggle night mode on/off and update colours
  void toggleNightMode();

  // Should be called periodically to refresh dynamic labels.  It reads
  // values from state.h and updates UI elements if they changed.
  void tick();
}
