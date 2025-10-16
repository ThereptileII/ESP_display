/*
 * canbus.cpp
 *
 * Implements CAN bridge communication for the Marine Dashboard.  The
 * bridge communicates using the SLCAN protocol over a UART.  Received
 * lines are parsed into CAN frames, NMEA2000 PGNs are extracted,
 * decoded and used to update global state.  Autopilot commands are
 * transmitted as PGN 127237 frames via the same bridge.
 */

#include "canbus.h"
#include "config.h"
#include "state.h"

#include <Arduino.h>

namespace canbus {
  using namespace config;

  // We assume the bridge appears as Serial2
  static HardwareSerial &bridge = Serial2;

  // Internal buffer for accumulating characters from SLCAN
  static String slcanLine;

  // Helper: parse SLCAN 'T' frame. Returns true on success.
  static bool parseSlcan(const String &line, uint32_t &id, uint8_t &len, uint8_t data[8]) {
    if (line.length() < 11) return false;
    if (line.charAt(0) != 'T') return false;
    char idStr[9] = {0};
    for (int i = 0; i < 8; i++) idStr[i] = line.charAt(1 + i);
    id = strtoul(idStr, nullptr, 16);
    char lenCh = line.charAt(9);
    if (lenCh < '0' || lenCh > '8') return false;
    len = (uint8_t)(lenCh - '0');
    int expected = 10 + len * 2;
    if (line.length() < expected) return false;
    for (uint8_t i = 0; i < len; i++) {
      char b0 = line.charAt(10 + i * 2);
      char b1 = line.charAt(11 + i * 2);
      char buf[3] = {b0, b1, 0};
      data[i] = (uint8_t)strtoul(buf, nullptr, 16);
    }
    return true;
  }

  // Extract PGN from 29‑bit ID according to NMEA2000 rules
  static uint32_t pgnFromId(uint32_t id) {
    uint8_t PF = (id >> 16) & 0xFF;
    uint8_t PS = (id >> 8)  & 0xFF;
    if (PF < 240) {
      return (uint32_t)PF << 8;
    } else {
      return ((uint32_t)PF << 8) | PS;
    }
  }

  // Autopilot TX helpers
  static uint32_t makeN2kId(uint32_t pgn, uint8_t src, uint8_t prio, uint8_t dst) {
    uint8_t PF = (pgn >> 8) & 0xFF;
    uint8_t PS = pgn & 0xFF;
    if (PF < 240) {
      return ((uint32_t)prio << 26) | ((uint32_t)PF << 16) | ((uint32_t)dst << 8) | src;
    } else {
      return ((uint32_t)prio << 26) | ((uint32_t)PF << 16) | ((uint32_t)PS << 8) | src;
    }
  }

  static void sendSlcan(uint32_t id, uint8_t len, const uint8_t data[8]) {
    char buf[64];
    int n = 0;
    n += snprintf(buf + n, sizeof(buf) - n, "T%08lX%1u", (unsigned long)id, len);
    for (uint8_t i = 0; i < len; i++) {
      n += snprintf(buf + n, sizeof(buf) - n, "%02X", data[i]);
    }
    buf[n++] = '\r';
    buf[n] = '\0';
    bridge.print(buf);
  }

  void beginSerial() {
    bridge.begin(CAN_BRIDGE_BAUD, SERIAL_8N1, CAN_BRIDGE_RX_PIN, CAN_BRIDGE_TX_PIN);
  }

  // PGN-specific decoders
  static void handlePGN128259(uint8_t len, const uint8_t d[8]) {
    // Speed Water Referenced: 0.01 m/s resolution (signed)
    if (len < 3) return;
    int16_t raw = (int16_t)(d[2] << 8 | d[1]);
    if (raw != 0x7FFF) {
      float mps = raw * 0.01f;
      float kts = mps * 1.943844f;
      g_STW_kts = kts;
    }
  }

  static void handlePGN127488(uint8_t len, const uint8_t d[8]) {
    // Engine Parameters Rapid Update: RPM in 0.25 RPM units at bytes 2-3
    if (len < 4) return;
    uint16_t raw = (uint16_t)(d[3] << 8 | d[2]);
    if (raw != 0xFFFF) {
      g_RPM = (int)floorf(raw * 0.25f + 0.5f);
    }
  }

  static void handlePGN127493(uint8_t len, const uint8_t d[8]) {
    // Transmission: gear information often stored in byte1 bits
    if (len < 2) return;
    uint8_t gs = d[1] & 0x03;
    char g = 'N';
    if (gs == 1) g = 'D';
    else if (gs == 2) g = 'R';
    g_Gear = g;
  }

  static void handlePGN127508(uint8_t len, const uint8_t d[8]) {
    // Battery Status: instance, voltage (0.01 V), current (0.1 A)
    if (len < 5) return;
    int16_t v_raw = (int16_t)(d[2] << 8 | d[1]);
    int16_t c_raw = (int16_t)(d[4] << 8 | d[3]);
    if (v_raw != 0x7FFF) {
      g_Batt_V = v_raw * 0.01f;
    }
    if (c_raw != 0x7FFF) {
      g_RegenA = c_raw * 0.1f;
    }
  }

  static void handlePGN127506(uint8_t len, const uint8_t d[8]) {
    // DC Detailed Status: may contain SOC percentage
    if (len < 2) return;
    uint8_t soc = d[1];
    if (soc != 0xFF) {
      g_SOC_pct = soc;
      g_SOC2_pct = soc;
    }
  }

  static void handlePGN130306(uint8_t len, const uint8_t d[8]) {
    // Wind Data: speed 0.01 m/s in bytes1-2, angle 0.0001 rad in bytes3-4, reference in byte5
    if (len < 6) return;
    int16_t sp_raw  = (int16_t)(d[2] << 8 | d[1]);
    int16_t ang_raw = (int16_t)(d[4] << 8 | d[3]);
    uint8_t ref      = d[5];
    if (sp_raw != 0x7FFF && ang_raw != 0x7FFF) {
      float sp_ms  = sp_raw * 0.01f;
      float ang_deg = (ang_raw * 0.0001f) * 57.2957795f;
      if (ang_deg < 0) ang_deg += 360.0f;
      if (ref == 1) { // apparent
        g_AWS_ms  = sp_ms;
        g_AWA_deg = ang_deg;
      } else if (ref == 2 || ref == 3) { // true
        g_TWS_ms  = sp_ms;
        g_TWA_deg = ang_deg;
      }
    }
  }

  static void handlePGN127250(uint8_t len, const uint8_t d[8]) {
    // Vessel Heading: 0.0001 rad at bytes1-2
    if (len < 3) return;
    int16_t hraw = (int16_t)(d[2] << 8 | d[1]);
    if (hraw != 0x7FFF) {
      float deg = (hraw * 0.0001f) * 57.2957795f;
      if (deg < 0) deg += 360.0f;
      g_hdg_deg = deg;
      // if AP not engaged and no setpoint yet, default to current heading
      if (!g_apEngaged && g_set_deg == 0.0f) g_set_deg = deg;
    }
  }

  static void handlePGN127237(uint8_t len, const uint8_t d[8]) {
    // Heading/Track Control: interpretation may be vendor specific
    if (len < 4) return;
    uint8_t mode = d[0];
    uint8_t en   = d[1];
    int16_t sp   = (int16_t)(d[3] << 8 | d[2]);
    if (mode <= 3) g_apMode = static_cast<APMode>(mode);
    g_apEngaged = (en != 0);
    if (sp != 0x7FFF) {
      float deg = (sp * 0.0001f) * 57.2957795f;
      if (deg < 0) deg += 360.0f;
      g_set_deg = deg;
    }
  }

  static void handlePGN127245(uint8_t len, const uint8_t d[8]) {
    // Rudder angle: 0.0001 rad at bytes0-1
    if (len < 2) return;
    int16_t raw = (int16_t)(d[1] << 8 | d[0]);
    if (raw != 0x7FFF) {
      float deg = (raw * 0.0001f) * 57.2957795f;
      g_rudder_deg = deg;
    }
  }

  // Main PGN dispatch
  static void dispatchPGN(uint32_t pgn, uint8_t len, const uint8_t d[8]) {
    switch (pgn) {
      case 128259: handlePGN128259(len, d); break;
      case 127488: handlePGN127488(len, d); break;
      case 127493: handlePGN127493(len, d); break;
      case 127508: handlePGN127508(len, d); break;
      case 127506: handlePGN127506(len, d); break;
      case 130306: handlePGN130306(len, d); break;
      case 127250: handlePGN127250(len, d); break;
      case 127237: handlePGN127237(len, d); break;
      case 127245: handlePGN127245(len, d); break;
      default: break;
    }
  }

  // Process available lines from SLCAN bridge
  void process() {
    while (bridge.available()) {
      char c = bridge.read();
      if (c == '\r' || c == '\n') {
        if (slcanLine.length() > 0) {
          uint32_t id; uint8_t len; uint8_t data[8];
          if (parseSlcan(slcanLine, id, len, data)) {
            uint32_t pgn = pgnFromId(id);
            dispatchPGN(pgn, len, data);
          }
          slcanLine = "";
        }
      } else {
        if (slcanLine.length() < 80) slcanLine += c;
        else slcanLine = "";
      }
    }
  }

  // Transmit autopilot command (engage/standby + mode + setpoint)
  void transmitApCommand(bool engage, APMode mode, float set_deg) {
    // update local state variables
    g_apEngaged = engage;
    g_apMode    = mode;
    g_set_deg   = fmodf(set_deg + 360.0f, 360.0f);
    // convert setpoint to 0.0001 rad
    int16_t sp = (int16_t)roundf((g_set_deg * 0.0174532925f) * 10000.0f);
    uint8_t d[8] = {0};
    d[0] = (uint8_t)mode;
    d[1] = engage ? 1 : 0;
    d[2] = (uint8_t)(sp & 0xFF);
    d[3] = (uint8_t)((sp >> 8) & 0xFF);
    uint32_t id = makeN2kId(127237, N2K_SRC_ADDR, N2K_PRIORITY, 0xFF);
    sendSlcan(id, 8, d);
  }

  void adjustApSetPoint(int delta) {
    g_set_deg = fmodf(g_set_deg + delta + 360.0f, 360.0f);
    // send update
    int16_t sp = (int16_t)roundf((g_set_deg * 0.0174532925f) * 10000.0f);
    uint8_t d[8] = {0};
    d[0] = (uint8_t)g_apMode;
    d[1] = g_apEngaged ? 1 : 0;
    d[2] = (uint8_t)(sp & 0xFF);
    d[3] = (uint8_t)((sp >> 8) & 0xFF);
    uint32_t id = makeN2kId(127237, N2K_SRC_ADDR, N2K_PRIORITY, 0xFF);
    sendSlcan(id, 8, d);
  }
} // namespace canbus
