MarineDashboard P4 v4 (ESP32-P4 + JD9365 @ 800x1280, LVGL v8) — Orbitron font integration

Put your generated font files into:  MarineDashboard_P4_v4/fonts/
  - orbitron_48_900.c
  - orbitron_32_800.c
  - orbitron_20_700.c
  - orbitron_16_600.c

The code auto-detects these and uses Orbitron; otherwise it falls back to Montserrat.

PowerShell commands (merge Orbitron + Segoe UI Symbol to include ° ± µ × ÷ and arrows):

# 48px / 900
lv_font_conv --format lvgl --bpp 4 --size 48 `
  --lv-font-name orbitron_48_900 `
  --font ".\static\Orbitron-Black.ttf" `
    -r 0x20-0x7E -r 0x00C5 -r 0x00C4 -r 0x00D6 -r 0x00E5 -r 0x00E4 -r 0x00F6 `
  --font "C:\Windows\Fonts\seguisym.ttf" `
    -r 0x00B0 -r 0x00B1 -r 0x00B5 -r 0x00D7 -r 0x00F7 -r 0x2190-0x2193 `
  -o .\fonts\orbitron_48_900.c

Repeat for 32/800, 20/700, 16/600 with respective TTFs and output names.

Hardware:
- ESP32-P4 HMI (JC8012P4A1), JD9365 MIPI panel, GSL3680 touch
- Landscape via LVGL SW rotation
- Stripe-based DMA flush with bounce buffer

UI:
- Pages: Engine (RPM/Power/Batt), Battery (logging), Wind (spd/angle)
- Pull-down overlay: Autopilot skeleton
- Top-right night-mode button

NMEA2000:
- Reads SLCAN 'T...' frames from UART CAN-bridge
- PGN 127508 (battery volts), 130306 (wind)

Vendor sources expected in the project folder (compile with sketch):
  esp_lcd_jd9365.c/.h, jd9365_lcd.cpp/.h
  esp_lcd_touch.c/.h, esp_lcd_gsl3680.c/.h, gsl3680_touch.cpp/.h, gsl_point_id.c/.h
