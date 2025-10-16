/*
 * Marine Dashboard
 *
 * This Arduino sketch drives a 10.1‑inch ESP32‑P4 powered display module.
 * It implements a multi‑page dashboard for monitoring vessel systems over
 * NMEA2000 and provides a slide‑down autopilot controller.  A persistent
 * toggle button lets the user enable night mode, switching all text
 * colours to a dark red.  Measurements such as battery voltage and
 * wind speed are logged to the SD card and stored in circular buffers
 * for later graphing.  Autopilot commands are transmitted over
 * NMEA2000 via a CAN bridge and PGNs are parsed from the same bus.
 *
 * The sketch is organised into several modules to keep the code easy to
 * read and maintain:
 *   config.h       – compile‑time settings and colour definitions
 *   state.h        – global state variables used throughout the project
 *   ui.h/cpp       – user interface creation, night mode and autopilot UI
 *   canbus.h/cpp   – CAN bridge integration and NMEA2000 PGN parsing
 *   logging.h/cpp  – SD logging and ring buffer management
 *   display_init.h/cpp – display and touch initialisation for the JC8012P4A1 board
 *
 * Author: ChatGPT
 */

#include "config.h"
#include "state.h"
#include "ui.h"
#include "canbus.h"
#include "logging.h"
#include "display_init.h"

void setup() {
  Serial.begin(115200);
  // initialise CAN bridge serial port defined in canbus.cpp
  canbus::beginSerial();

  // initialise SD card (creates log files if missing)
  logging::init();

  // initialise display, LVGL and touch
  display_init::initDisplayAndTouch();
  // build the UI and register callbacks
  ui::init();
}

void loop() {
  // allow LVGL to process tasks
  lv_timer_handler();
  // read frames from CAN bridge and update state/UI
  canbus::process();
  // perform periodic sampling and SD logging
  logging::tick();
  // small delay to yield CPU
  delay(4);
}