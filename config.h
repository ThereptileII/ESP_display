/*
 * config.h
 *
 * This header defines compile‑time configuration values and colour
 * constants used throughout the Marine Dashboard.  Separating
 * these values into a single file makes it easy to adjust the
 * appearance and behaviour without modifying other modules.
 */

#pragma once

#include <lvgl.h>

// Display resolution for the Surenoo JC8012P4A1 module (rotated to landscape)
static const int DISPLAY_WIDTH  = 1280;
static const int DISPLAY_HEIGHT = 800;

// Sampling intervals (ms) and series lengths
static const uint32_t SAMPLE_INTERVAL_MS = 3500;    // 3.5 seconds per sample
static const uint16_t SERIES_LENGTH      = 1024;    // number of points per time horizon

// GPIO pin assignments for the CAN bridge UART (adjust as required)
static const int CAN_BRIDGE_RX_PIN = 16;
static const int CAN_BRIDGE_TX_PIN = 17;
static const unsigned long CAN_BRIDGE_BAUD = 115200;

// SD card settings
// Set USE_SD_MMC to 1 if your hardware uses SD_MMC (4‑bit), otherwise SPI
static const bool USE_SD_MMC = false;
static const int  SD_CS_PIN  = 13; // chip select for SPI SD

// Colour palette for day mode
namespace DayColours {
  static const lv_color_t BG_MAIN    = lv_color_hex(0x101319); // dark blue/gray
  static const lv_color_t CARD       = lv_color_hex(0x181D25);
  static const lv_color_t CARD_DARK  = lv_color_hex(0x14181F);
  static const lv_color_t CYAN       = lv_color_hex(0x51D9FB);
  static const lv_color_t ORANGE     = lv_color_hex(0xFBB451);
  static const lv_color_t GREEN      = lv_color_hex(0x2BEE2B);
  static const lv_color_t WHITE      = lv_color_hex(0xFAFAFA);
}

// Colour palette for night mode – dark red text on same dark background
namespace NightColours {
  static const lv_color_t BG_MAIN    = DayColours::BG_MAIN;       // same background
  static const lv_color_t CARD       = DayColours::CARD;           // same card backgrounds
  static const lv_color_t CARD_DARK  = DayColours::CARD_DARK;
  static const lv_color_t PRIMARY    = lv_color_hex(0x8B0000);     // dark red for all text
}

// Glow opacities (0–255) for day mode
namespace DayGlow {
  static const uint8_t CYAN   = 76;  // ~30% opacity
  static const uint8_t ORANGE = 76;
  static const uint8_t GREEN  = 76;
  static const uint8_t WHITE  = 51;  // ~20%
}

// Glow opacity for night mode – use dark red glow
namespace NightGlow {
  static const uint8_t PRIMARY = 76;
}

// Page padding and styling constants
static const int PADDING = 20;
static const int CARD_RADIUS = 28;
static const int CARD_SHADOW_WIDTH = 18;
static const uint8_t CARD_SHADOW_OPA = 40;

// Paths for log files on the SD card
static const char* LOG_BATTERY_PATH = "/batt_v.csv";
static const char* LOG_WIND_PATH    = "/wind.csv";

// Autopilot configuration
// Free source address for our AP messages
static const uint8_t N2K_SRC_ADDR = 0x23;
static const uint8_t N2K_PRIORITY = 3;
