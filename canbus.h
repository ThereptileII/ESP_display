/*
 * canbus.h
 *
 * Provides functions to interface with a serial CAN bridge (e.g. MCP2515
 * SLCAN adapter) and parse NMEA2000 PGNs.  Autopilot commands are
 * transmitted via PGN 127237 using helper functions.  All parsing
 * updates the global state variables defined in state.h.  This
 * module does not directly touch the UI; instead, the UI polls
 * state.h or registers callbacks for update.
 */

#pragma once

#include <Arduino.h>
#include "state.h"

namespace canbus {
  // Initialise the CAN bridge serial port
  void beginSerial();

  // Process all available lines from the CAN bridge and update state
  void process();

  // Transmit autopilot engage/standby and setpoint messages.  This
  // function uses PGN 127237 in a simple format: mode, engage flag,
  // and setpoint encoded as 0.0001 rad.  See implementation for
  // details.  You can adapt it for your specific autopilot PGNs.
  void transmitApCommand(bool engage, APMode mode, float set_deg);

  // Adjust autopilot setpoint by delta degrees and transmit
  void adjustApSetPoint(int delta);
}
