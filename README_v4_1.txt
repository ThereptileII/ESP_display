MarineDashboard P4 v4.1 (ESP32-P4 + JD9365 @ 800x1280, LVGL v8)
- LVGL v8 API fixed (uses lv_tick_inc / legacy indev timer fallback by default;
  enable LVGL_USE_INDEV_SET_READ_PERIOD=1 if your LVGL build provides the helper)
- Fonts compiled as separate C units (no #include .c)
- Landscape + touch mapping + stripe/bounce flush

Place vendor files in the same folder:
  esp_lcd_jd9365.c / esp_lcd_jd9365.h
  jd9365_lcd.cpp / jd9365_lcd.h
  esp_lcd_touch.c / esp_lcd_touch.h
  esp_lcd_gsl3680.c / esp_lcd_gsl3680.h
  gsl3680_touch.cpp / gsl3680_touch.h
  gsl_point_id.c / gsl_point_id.h
  pins_config.h (optional override for TP pins)

Fonts: put generated LVGL .c files into fonts/
  orbitron_48_900.c, orbitron_32_800.c, orbitron_20_700.c, orbitron_16_600.c
They must include: #include "lvgl.h" (use --lv-include "lvgl.h" in lv_font_conv)
