#pragma once
// -------- Build target: LVGL v8 (Arduino Library Manager: "lvgl" 8.x) --------

// 0=portrait, 1=landscape (rotated 90° clockwise)
#ifndef ORIENTATION_MODE
#define ORIENTATION_MODE 1
#endif

// ---------- CAN-bridge (SLCAN text "TxxxxxxxxLdd...") over UART ----------
#ifndef CANBRIDGE_UART
  #define CANBRIDGE_UART  Serial2
#endif
#ifndef CANBRIDGE_BAUD
  #define CANBRIDGE_BAUD  115200
#endif
#ifndef CANBRIDGE_RX
  #define CANBRIDGE_RX    16
#endif
#ifndef CANBRIDGE_TX
  #define CANBRIDGE_TX    17
#endif

// ---------- SD card ----------
#define USE_SD_MMC 1   // 1=on-board TF slot with SD_MMC, 0=classic SD+SPI

// ---------- Touch orientation compensation (after vendor driver's rotation) ----------
#ifndef TOUCH_SWAP_XY
#define TOUCH_SWAP_XY   1
#endif
#ifndef TOUCH_INVERT_X
#define TOUCH_INVERT_X  1
#endif
#ifndef TOUCH_INVERT_Y
#define TOUCH_INVERT_Y  0
#endif

// ---------- Colors (Marine palette) ----------
#define CLR_BG_MAIN   0x131821
#define CLR_BG_CARD_A 0x171C26
#define CLR_CYAN      0x2FD3E6
#define CLR_ORANGE    0xF0A33C
#define CLR_GREEN     0x3DE06A
#define CLR_NEARWHITE 0xFAFAFA
#define CLR_NIGHT_RED 0x7F0000
