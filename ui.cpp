/*
 * ui.cpp
 *
 * Implements the graphical user interface for the Marine Dashboard.
 * Pages are built using LVGL and follow the dark, high‑contrast
 * aesthetic specified in the design guidelines.  Dynamic content
 * updates each loop based on values from state.h.  Night mode
 * toggles text colours to dark red.  An autopilot controller sheet
 * slides down from the top and provides heading adjustments.
 */

#include "ui.h"
#include "config.h"
#include "state.h"
#include "canbus.h"
#include "logging.h"

#include <Arduino.h>
#include <vector>

using namespace config;

namespace ui {

  // Forward declarations for internal helpers
  static void styles_init();
  static void buildPages();
  static void buildSpeedPage();
  static void buildOverviewPage();
  static void buildWindPage();
  static void buildApSheet();
  static void updateApSheet();
  static void apShow();
  static void apHide();
  static void buildNightButton();
  static void updateNightButton();
  static void applyNightColours();

  // LVGL styles
  static lv_style_t style_screen;
  static lv_style_t style_card;
  static lv_style_t style_label_small;
  static lv_style_t style_unit;
  static lv_style_t style_val_cyan;
  static lv_style_t style_val_orange;
  static lv_style_t style_val_green;
  static lv_style_t style_val_white;

  // TabView and pages
  static lv_obj_t *tabview;
  static lv_obj_t *page_speed;
  static lv_obj_t *page_overview;
  static lv_obj_t *page_wind;

  // Speed page widgets
  static lv_obj_t *lbl_stw_value;
  static lv_obj_t *lbl_rpm_s_value;
  static lv_obj_t *lbl_gear_value;
  static lv_obj_t *lbl_regen_value;
  static lv_obj_t *lbl_soc_s_value;

  // Overview widgets
  static lv_obj_t *lbl_rpm_o_value;
  static lv_obj_t *lbl_remP_value;
  static lv_obj_t *lbl_batt_value;
  static lv_obj_t *lbl_pdraw_value;
  static lv_obj_t *lbl_soc_o_value;
  static lv_obj_t *lbl_dist_value;
  static lv_obj_t *lbl_ttg_value;

  // Wind widgets
  static lv_obj_t *lbl_ws_center;
  static lv_obj_t *lbl_twa_value;
  static lv_obj_t *lbl_awa_value;
  static lv_obj_t *lbl_wmax_value;
  static lv_obj_t *lbl_wtws_value;
  static lv_obj_t *lbl_waws_value;
  static lv_obj_t *lbl_wmin_value;
  static lv_meter_indicator_t *wind_needle;
  static lv_meter_t *wind_meter;

  // Autopilot sheet widgets
  static lv_obj_t *ap_sheet;
  static lv_obj_t *ap_chip_mode;
  static lv_obj_t *ap_btn_engage;
  static lv_obj_t *ap_lbl_hdg;
  static lv_obj_t *ap_lbl_set;
  static lv_obj_t *ap_lbl_xte;
  static lv_obj_t *ap_btnm_coarse;
  static lv_obj_t *ap_btnm_fine;

  // Night mode button
  static lv_obj_t *night_btn;

  // Utility: apply glow effect
  static void apply_glow(lv_obj_t *o, lv_color_t c, uint8_t opa) {
    lv_obj_set_style_shadow_width(o, 20, 0);
    lv_obj_set_style_shadow_color(o, c, 0);
    lv_obj_set_style_shadow_opa(o, opa, 0);
    lv_obj_set_style_shadow_spread(o, 0, 0);
  }

  // Build a card container
  static lv_obj_t *card(lv_obj_t *parent, int x, int y, int w, int h) {
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_remove_style_all(c);
    lv_obj_add_style(c, &style_card, 0);
    lv_obj_set_size(c, w, h);
    lv_obj_set_pos(c, x, y);
    lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    return c;
  }

  // Create a small label at a specific position
  static lv_obj_t *label_at(lv_obj_t *parent, const char *txt, lv_style_t *st, int x, int y) {
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_add_style(l, st, 0);
    lv_label_set_text(l, txt);
    lv_obj_set_pos(l, x, y);
    return l;
  }

  // Styles initialisation
  static void styles_init() {
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, DayColours::BG_MAIN);
    lv_style_set_text_color(&style_screen, DayColours::WHITE);
    lv_style_set_text_font(&style_screen, &lv_font_montserrat_16);

    lv_style_init(&style_card);
    lv_style_set_radius(&style_card, CARD_RADIUS);
    lv_style_set_bg_color(&style_card, DayColours::CARD);
    lv_style_set_bg_grad_color(&style_card, DayColours::CARD_DARK);
    lv_style_set_bg_grad_dir(&style_card, LV_GRAD_DIR_VER);
    lv_style_set_border_width(&style_card, 0);
    lv_style_set_pad_all(&style_card, 24);
    lv_style_set_shadow_width(&style_card, CARD_SHADOW_WIDTH);
    lv_style_set_shadow_opa(&style_card, CARD_SHADOW_OPA);
    lv_style_set_shadow_color(&style_card, lv_color_hex(0x000000));

    lv_style_init(&style_label_small);
    lv_style_set_text_font(&style_label_small, &lv_font_montserrat_16);
    lv_style_set_text_color(&style_label_small, DayColours::WHITE);

    lv_style_init(&style_unit);
    lv_style_set_text_font(&style_unit, &lv_font_montserrat_20);
    lv_style_set_text_color(&style_unit, DayColours::WHITE);

    lv_style_init(&style_val_cyan);
    lv_style_set_text_font(&style_val_cyan, &lv_font_montserrat_40);
    lv_style_set_text_color(&style_val_cyan, DayColours::CYAN);

    lv_style_init(&style_val_orange);
    lv_style_set_text_font(&style_val_orange, &lv_font_montserrat_40);
    lv_style_set_text_color(&style_val_orange, DayColours::ORANGE);

    lv_style_init(&style_val_green);
    lv_style_set_text_font(&style_val_green, &lv_font_montserrat_40);
    lv_style_set_text_color(&style_val_green, DayColours::GREEN);

    lv_style_init(&style_val_white);
    lv_style_set_text_font(&style_val_white, &lv_font_montserrat_40);
    lv_style_set_text_color(&style_val_white, DayColours::WHITE);
  }

  // Construct the UI
  void init() {
    styles_init();
    // apply screen style to root
    lv_obj_add_style(lv_scr_act(), &style_screen, 0);
    // create TabView for pages and hide tab bar for clean swiping
    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 1);
    lv_tabview_set_tab_bar_hidden(tabview, true);
    lv_tabview_set_anim_time(tabview, 220);
    buildPages();
    // Build autopilot gesture handle
    lv_obj_t *handle = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(handle);
    lv_obj_set_size(handle, DISPLAY_WIDTH, 24);
    lv_obj_set_pos(handle, 0, 0);
    lv_obj_add_flag(handle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(handle, [](lv_event_t *e){
      if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_event_get_indev(e));
        lv_point_t p;
        lv_indev_get_point(lv_event_get_indev(e), &p);
        if (dir == LV_DIR_BOTTOM && p.y < 64) {
          apShow();
        }
      } else if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        apShow();
      }
    }, LV_EVENT_ALL, NULL);
    // build persistent night mode toggle button
    buildNightButton();
  }

  static void buildPages() {
    buildSpeedPage();
    buildOverviewPage();
    buildWindPage();
    // select first page
    lv_tabview_set_act(tabview, 0, LV_ANIM_OFF);
  }

  // Build Speed Focus page
  static void buildSpeedPage() {
    page_speed = lv_tabview_add_tab(tabview, "speed");
    // top big tile
    int topH = DISPLAY_HEIGHT - (PADDING * 3) - 220;
    lv_obj_t *tile_stw = card(page_speed, PADDING, PADDING, DISPLAY_WIDTH - 2 * PADDING, topH);
    lv_obj_t *lbl_stw_label = label_at(tile_stw, "Stw:", &style_label_small, 8, 4);
    // center big value
    lv_obj_t *lbl_value = lv_label_create(tile_stw);
    lbl_stw_value = lbl_value;
    lv_obj_set_style_text_font(lbl_value, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lbl_value, DayColours::WHITE, 0);
    apply_glow(lbl_value, DayColours::WHITE, DayGlow::WHITE);
    lv_label_set_text_fmt(lbl_value, "%.1f", g_STW_kts);
    lv_obj_align(lbl_value, LV_ALIGN_CENTER, -60, 0);
    // units
    lv_obj_t *lbl_unit = lv_label_create(tile_stw);
    lv_obj_add_style(lbl_unit, &style_unit, 0);
    lv_label_set_text(lbl_unit, "kts");
    apply_glow(lbl_unit, DayColours::WHITE, DayGlow::WHITE);
    lv_obj_align_to(lbl_unit, lbl_value, LV_ALIGN_OUT_RIGHT_BOTTOM, 16, -6);
    // bottom row of four tiles
    int rowH = 220;
    int tileW = (DISPLAY_WIDTH - (PADDING * 5)) / 4;
    int y = PADDING * 2 + topH;
    // RPM tile
    lv_obj_t *tile_rpm = card(page_speed, PADDING, y, tileW, rowH);
    lv_obj_t *lbl_rpm_label = label_at(tile_rpm, "RPM:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_rpm_label, DayColours::CYAN, 0);
    lv_obj_t *lbl_rpm_value = lv_label_create(tile_rpm);
    lbl_rpm_s_value = lbl_rpm_value;
    lv_obj_add_style(lbl_rpm_value, &style_val_cyan, 0);
    apply_glow(lbl_rpm_value, DayColours::CYAN, DayGlow::CYAN);
    lv_label_set_text_fmt(lbl_rpm_value, "%d", g_RPM);
    lv_obj_align(lbl_rpm_value, LV_ALIGN_CENTER, 0, 20);
    // Gear tile
    lv_obj_t *tile_gear = card(page_speed, PADDING * 2 + tileW, y, tileW, rowH);
    lv_obj_t *lbl_gear_label = label_at(tile_gear, "Gear:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_gear_label, DayColours::ORANGE, 0);
    lv_obj_t *lbl_gear_val = lv_label_create(tile_gear);
    lbl_gear_value = lbl_gear_val;
    lv_obj_add_style(lbl_gear_val, &style_val_orange, 0);
    apply_glow(lbl_gear_val, DayColours::ORANGE, DayGlow::ORANGE);
    lv_label_set_text_fmt(lbl_gear_val, "%c", g_Gear);
    lv_obj_align(lbl_gear_val, LV_ALIGN_CENTER, 0, 20);
    // Regen tile
    lv_obj_t *tile_regen = card(page_speed, PADDING * 3 + tileW * 2, y, tileW, rowH);
    lv_obj_t *lbl_regen_label = label_at(tile_regen, "Regen:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_regen_label, DayColours::ORANGE, 0);
    lv_obj_t *lbl_regen_val = lv_label_create(tile_regen);
    lbl_regen_value = lbl_regen_val;
    lv_obj_add_style(lbl_regen_val, &style_val_orange, 0);
    apply_glow(lbl_regen_val, DayColours::ORANGE, DayGlow::ORANGE);
    if (fabs(g_RegenA) < 0.05f) lv_label_set_text(lbl_regen_val, "-");
    else lv_label_set_text_fmt(lbl_regen_val, "%.0f A", fabs(g_RegenA));
    lv_obj_align(lbl_regen_val, LV_ALIGN_CENTER, 0, 20);
    // SOC tile
    lv_obj_t *tile_soc = card(page_speed, PADDING * 4 + tileW * 3, y, tileW, rowH);
    lv_obj_t *lbl_soc_label = label_at(tile_soc, "SOC:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_soc_label, DayColours::WHITE, 0);
    lv_obj_t *lbl_soc_val = lv_label_create(tile_soc);
    lbl_soc_s_value = lbl_soc_val;
    lv_obj_add_style(lbl_soc_val, &style_val_white, 0);
    apply_glow(lbl_soc_val, DayColours::WHITE, DayGlow::WHITE);
    lv_label_set_text_fmt(lbl_soc_val, "%d %%", g_SOC_pct);
    lv_obj_align(lbl_soc_val, LV_ALIGN_CENTER, 0, 20);
  }

  // Build Overview page
  static void buildOverviewPage() {
    page_overview = lv_tabview_add_tab(tabview, "overview");
    int row1H = 320;
    int rowH = (DISPLAY_HEIGHT - (PADDING * 4) - row1H) / 2;
    int colW = (DISPLAY_WIDTH - (PADDING * 3)) / 2;
    // RPM (row1 col0)
    lv_obj_t *tile_rpm = card(page_overview, PADDING, PADDING, colW, row1H);
    lv_obj_t *lbl_rpm_label = label_at(tile_rpm, "RPM:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_rpm_label, DayColours::CYAN, 0);
    lv_obj_t *lbl_rpm_val = lv_label_create(tile_rpm);
    lbl_rpm_o_value = lbl_rpm_val;
    lv_obj_add_style(lbl_rpm_val, &style_val_cyan, 0);
    apply_glow(lbl_rpm_val, DayColours::CYAN, DayGlow::CYAN);
    lv_label_set_text_fmt(lbl_rpm_val, "%d", g_RPM);
    lv_obj_align(lbl_rpm_val, LV_ALIGN_CENTER, 0, 20);
    // Remaining power (row1 col1)
    lv_obj_t *tile_rem = card(page_overview, PADDING * 2 + colW, PADDING, colW, row1H);
    lv_obj_t *lbl_rem_label = label_at(tile_rem, "Remaining power:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_rem_label, DayColours::ORANGE, 0);
    lv_obj_t *lbl_rem_val = lv_label_create(tile_rem);
    lbl_remP_value = lbl_rem_val;
    lv_obj_add_style(lbl_rem_val, &style_val_orange, 0);
    apply_glow(lbl_rem_val, DayColours::ORANGE, DayGlow::ORANGE);
    lv_label_set_text_fmt(lbl_rem_val, "%.1f", g_Rem_kWh);
    lv_obj_align(lbl_rem_val, LV_ALIGN_CENTER, -40, 20);
    lv_obj_t *lbl_rem_unit = lv_label_create(tile_rem);
    lv_obj_add_style(lbl_rem_unit, &style_unit, 0);
    lv_label_set_text(lbl_rem_unit, "kwh");
    apply_glow(lbl_rem_unit, DayColours::ORANGE, DayGlow::ORANGE);
    lv_obj_align_to(lbl_rem_unit, lbl_rem_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 16, -6);
    // Battery (row2 col0 spanning two rows)
    int battY = PADDING * 2 + row1H;
    int battH = rowH * 2 + PADDING;
    lv_obj_t *tile_batt = card(page_overview, PADDING, battY, colW, battH);
    lv_obj_t *lbl_batt_label = label_at(tile_batt, "Battery voltage:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_batt_label, DayColours::GREEN, 0);
    lv_obj_t *lbl_batt_val = lv_label_create(tile_batt);
    lbl_batt_value = lbl_batt_val;
    lv_obj_add_style(lbl_batt_val, &style_val_green, 0);
    apply_glow(lbl_batt_val, DayColours::GREEN, DayGlow::GREEN);
    lv_label_set_text_fmt(lbl_batt_val, "%.0f", g_Batt_V);
    lv_obj_align(lbl_batt_val, LV_ALIGN_CENTER, -20, 40);
    lv_obj_t *lbl_batt_unit = lv_label_create(tile_batt);
    lv_obj_add_style(lbl_batt_unit, &style_unit, 0);
    lv_label_set_text(lbl_batt_unit, "v");
    apply_glow(lbl_batt_unit, DayColours::GREEN, DayGlow::GREEN);
    lv_obj_align_to(lbl_batt_unit, lbl_batt_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 12, -6);
    // Make battery tile clickable to open battery graph; callback not yet implemented
    lv_obj_add_event_cb(tile_batt, [](lv_event_t *e){
      // In the future this will open a battery history subpage
      // Placeholder: no action for now
    }, LV_EVENT_CLICKED, NULL);
    // Right column row2 (Power draw and SOC)
    int rcolX = PADDING * 2 + colW;
    int row2Y = PADDING * 2 + row1H;
    int smallW = (colW - PADDING) / 2;
    // Power draw
    lv_obj_t *tile_draw = card(page_overview, rcolX, row2Y, smallW, rowH);
    lv_obj_t *lbl_draw_label = label_at(tile_draw, "Power draw:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_draw_label, DayColours::WHITE, 0);
    lv_obj_t *lbl_draw_val = lv_label_create(tile_draw);
    lbl_pdraw_value = lbl_draw_val;
    lv_obj_add_style(lbl_draw_val, &style_val_white, 0);
    apply_glow(lbl_draw_val, DayColours::WHITE, DayGlow::WHITE);
    lv_label_set_text_fmt(lbl_draw_val, "%.1f", g_Pdraw_kW);
    lv_obj_align(lbl_draw_val, LV_ALIGN_CENTER, -30, 18);
    lv_obj_t *lbl_draw_unit = lv_label_create(tile_draw);
    lv_obj_add_style(lbl_draw_unit, &style_unit, 0);
    lv_label_set_text(lbl_draw_unit, "kw");
    apply_glow(lbl_draw_unit, DayColours::WHITE, DayGlow::WHITE);
    lv_obj_align_to(lbl_draw_unit, lbl_draw_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 12, -6);
    // SOC (overview)
    lv_obj_t *tile_soc = card(page_overview, rcolX + smallW + PADDING, row2Y, smallW, rowH);
    lv_obj_t *lbl_soc_label = label_at(tile_soc, "SOC:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_soc_label, DayColours::WHITE, 0);
    lv_obj_t *lbl_soc_val = lv_label_create(tile_soc);
    lbl_soc_o_value = lbl_soc_val;
    lv_obj_add_style(lbl_soc_val, &style_val_white, 0);
    apply_glow(lbl_soc_val, DayColours::WHITE, DayGlow::WHITE);
    lv_label_set_text_fmt(lbl_soc_val, "%d", g_SOC2_pct);
    lv_obj_align(lbl_soc_val, LV_ALIGN_CENTER, -20, 18);
    lv_obj_t *lbl_soc_unit = lv_label_create(tile_soc);
    lv_obj_add_style(lbl_soc_unit, &style_unit, 0);
    lv_label_set_text(lbl_soc_unit, "%");
    apply_glow(lbl_soc_unit, DayColours::WHITE, DayGlow::WHITE);
    lv_obj_align_to(lbl_soc_unit, lbl_soc_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 12, -6);
    // Right column row3 (Distance to go and Time to go)
    int row3Y = PADDING * 3 + row1H + rowH;
    lv_obj_t *tile_dist = card(page_overview, rcolX, row3Y, smallW, rowH);
    lv_obj_t *lbl_dist_label = label_at(tile_dist, "Distance to go:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_dist_label, DayColours::WHITE, 0);
    lv_obj_t *lbl_dist_val = lv_label_create(tile_dist);
    lbl_dist_value = lbl_dist_val;
    lv_obj_add_style(lbl_dist_val, &style_val_white, 0);
    apply_glow(lbl_dist_val, DayColours::WHITE, DayGlow::WHITE);
    lv_label_set_text_fmt(lbl_dist_val, "%d", g_Dist_kts);
    lv_obj_align(lbl_dist_val, LV_ALIGN_CENTER, -30, 18);
    lv_obj_t *lbl_dist_unit = lv_label_create(tile_dist);
    lv_obj_add_style(lbl_dist_unit, &style_unit, 0);
    lv_label_set_text(lbl_dist_unit, "kts");
    apply_glow(lbl_dist_unit, DayColours::WHITE, DayGlow::WHITE);
    lv_obj_align_to(lbl_dist_unit, lbl_dist_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 12, -6);
    lv_obj_t *tile_ttg = card(page_overview, rcolX + smallW + PADDING, row3Y, smallW, rowH);
    lv_obj_t *lbl_ttg_label = label_at(tile_ttg, "Time to go:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_ttg_label, DayColours::WHITE, 0);
    lv_obj_t *lbl_ttg_val = lv_label_create(tile_ttg);
    lbl_ttg_value = lbl_ttg_val;
    lv_obj_add_style(lbl_ttg_val, &style_val_white, 0);
    apply_glow(lbl_ttg_val, DayColours::WHITE, DayGlow::WHITE);
    lv_label_set_text_fmt(lbl_ttg_val, "%d", g_TTG_hrs);
    lv_obj_align(lbl_ttg_val, LV_ALIGN_CENTER, -30, 18);
    lv_obj_t *lbl_ttg_unit = lv_label_create(tile_ttg);
    lv_obj_add_style(lbl_ttg_unit, &style_unit, 0);
    lv_label_set_text(lbl_ttg_unit, "hrs");
    apply_glow(lbl_ttg_unit, DayColours::WHITE, DayGlow::WHITE);
    lv_obj_align_to(lbl_ttg_unit, lbl_ttg_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 12, -6);
  }

  // Build Wind page
  static void buildWindPage() {
    page_wind = lv_tabview_add_tab(tabview, "wind");
    // 4 columns × 2 rows layout
    int cols = 4;
    int rows = 2;
    int tileW = (DISPLAY_WIDTH - (PADDING * (cols + 1))) / cols;
    int tileH = (DISPLAY_HEIGHT - (PADDING * (rows + 1))) / rows;
    // meter at (0,0)
    lv_obj_t *tile_meter = card(page_wind, PADDING, PADDING, tileW, tileH);
    wind_meter = lv_meter_create(tile_meter);
    lv_obj_set_size(wind_meter, tileW - 40, tileH - 40);
    lv_obj_center(wind_meter);
    lv_meter_scale_t *scale = lv_meter_add_scale(wind_meter);
    lv_meter_set_scale_range(wind_meter, scale, 0, 360, 300, 120);
    lv_meter_set_scale_ticks(wind_meter, scale, 37, 2, 10, lv_color_hex(0xCCCCCC));
    lv_meter_set_scale_major_ticks(wind_meter, scale, 4, 4, 16, lv_color_hex(0xFFFFFF), 10);
    // coloured arcs
    lv_meter_add_arc(wind_meter, scale, 4, lv_palette_main(LV_PALETTE_RED), 0, 30);
    lv_meter_add_arc(wind_meter, scale, 4, lv_palette_main(LV_PALETTE_RED), 330, 30);
    lv_meter_add_arc(wind_meter, scale, 4, lv_palette_main(LV_PALETTE_GREEN), 60, 60);
    wind_needle = lv_meter_add_needle_line(wind_meter, scale, tileH / 2 - 50, DayColours::ORANGE, 4);
    // center label inside meter
    lbl_ws_center = lv_label_create(tile_meter);
    lv_obj_set_style_text_font(lbl_ws_center, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(lbl_ws_center, DayColours::WHITE, 0);
    apply_glow(lbl_ws_center, DayColours::WHITE, DayGlow::WHITE);
    lv_label_set_text_fmt(lbl_ws_center, "%.0f\nM/S", g_TWS_ms);
    lv_obj_align(lbl_ws_center, LV_ALIGN_CENTER, 0, 0);
    // top row (1,0) TWA
    int x = PADDING * 2 + tileW;
    int y = PADDING;
    // TWA tile
    lv_obj_t *tile_twa = card(page_wind, x, y, tileW, tileH);
    lv_obj_t *lbl_twa_label = label_at(tile_twa, "TWA:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_twa_label, DayColours::CYAN, 0);
    lv_obj_t *lbl_twa_val = lv_label_create(tile_twa);
    lbl_twa_value = lbl_twa_val;
    lv_obj_add_style(lbl_twa_val, &style_val_cyan, 0);
    apply_glow(lbl_twa_val, DayColours::CYAN, DayGlow::CYAN);
    lv_label_set_text_fmt(lbl_twa_val, "%.0f", g_TWA_deg);
    lv_obj_align(lbl_twa_val, LV_ALIGN_CENTER, -20, 20);
    lv_obj_t *lbl_twa_unit = lv_label_create(tile_twa);
    lv_obj_add_style(lbl_twa_unit, &style_unit, 0);
    lv_label_set_text(lbl_twa_unit, "°");
    lv_obj_align_to(lbl_twa_unit, lbl_twa_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, -6);
    // AWA tile
    x += PADDING + tileW;
    lv_obj_t *tile_awa = card(page_wind, x, y, tileW, tileH);
    lv_obj_t *lbl_awa_label = label_at(tile_awa, "AWA:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_awa_label, DayColours::WHITE, 0);
    lv_obj_t *lbl_awa_val = lv_label_create(tile_awa);
    lbl_awa_value = lbl_awa_val;
    lv_obj_add_style(lbl_awa_val, &style_val_white, 0);
    apply_glow(lbl_awa_val, DayColours::WHITE, DayGlow::WHITE);
    lv_label_set_text_fmt(lbl_awa_val, "%.0f", g_AWA_deg);
    lv_obj_align(lbl_awa_val, LV_ALIGN_CENTER, -20, 20);
    lv_obj_t *lbl_awa_unit = lv_label_create(tile_awa);
    lv_obj_add_style(lbl_awa_unit, &style_unit, 0);
    lv_label_set_text(lbl_awa_unit, "°");
    lv_obj_align_to(lbl_awa_unit, lbl_awa_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, -6);
    // MAX tile
    x += PADDING + tileW;
    lv_obj_t *tile_max = card(page_wind, x, y, tileW, tileH);
    lv_obj_t *lbl_max_label = label_at(tile_max, "MAX:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_max_label, DayColours::CYAN, 0);
    lv_obj_t *lbl_max_val = lv_label_create(tile_max);
    lbl_wmax_value = lbl_max_val;
    lv_obj_add_style(lbl_max_val, &style_val_cyan, 0);
    apply_glow(lbl_max_val, DayColours::CYAN, DayGlow::CYAN);
    lv_label_set_text_fmt(lbl_max_val, "%.0f", (g_TWS_max > 0 ? g_TWS_max : 0));
    lv_obj_align(lbl_max_val, LV_ALIGN_CENTER, -30, 20);
    lv_obj_t *lbl_max_unit = lv_label_create(tile_max);
    lv_obj_add_style(lbl_max_unit, &style_unit, 0);
    lv_label_set_text(lbl_max_unit, "M/S");
    lv_obj_align_to(lbl_max_unit, lbl_max_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 12, -6);
    // bottom row begins
    x = PADDING;
    y = PADDING * 2 + tileH;
    // blank tile at (0,1)
    lv_obj_t *tile_blank = card(page_wind, x, y, tileW, tileH);
    (void)tile_blank;
    // TWS tile
    x += PADDING + tileW;
    lv_obj_t *tile_tws = card(page_wind, x, y, tileW, tileH);
    lv_obj_t *lbl_tws_label = label_at(tile_tws, "TWS:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_tws_label, DayColours::GREEN, 0);
    lv_obj_t *lbl_tws_val = lv_label_create(tile_tws);
    lbl_wtws_value = lbl_tws_val;
    lv_obj_add_style(lbl_tws_val, &style_val_green, 0);
    apply_glow(lbl_tws_val, DayColours::GREEN, DayGlow::GREEN);
    lv_label_set_text_fmt(lbl_tws_val, "%.0f", g_TWS_ms);
    lv_obj_align(lbl_tws_val, LV_ALIGN_CENTER, -40, 18);
    lv_obj_t *lbl_tws_unit = lv_label_create(tile_tws);
    lv_obj_add_style(lbl_tws_unit, &style_unit, 0);
    lv_label_set_text(lbl_tws_unit, "M/S");
    lv_obj_align_to(lbl_tws_unit, lbl_tws_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 12, -6);
    // AWS tile
    x += PADDING + tileW;
    lv_obj_t *tile_aws = card(page_wind, x, y, tileW, tileH);
    lv_obj_t *lbl_aws_label = label_at(tile_aws, "AWS:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_aws_label, DayColours::ORANGE, 0);
    lv_obj_t *lbl_aws_val = lv_label_create(tile_aws);
    lbl_waws_value = lbl_aws_val;
    lv_obj_add_style(lbl_aws_val, &style_val_orange, 0);
    apply_glow(lbl_aws_val, DayColours::ORANGE, DayGlow::ORANGE);
    lv_label_set_text_fmt(lbl_aws_val, "%.0f", g_AWS_ms);
    lv_obj_align(lbl_aws_val, LV_ALIGN_CENTER, -40, 18);
    lv_obj_t *lbl_aws_unit = lv_label_create(tile_aws);
    lv_obj_add_style(lbl_aws_unit, &style_unit, 0);
    lv_label_set_text(lbl_aws_unit, "M/S");
    lv_obj_align_to(lbl_aws_unit, lbl_aws_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 12, -6);
    // MIN tile
    x += PADDING + tileW;
    lv_obj_t *tile_min = card(page_wind, x, y, tileW, tileH);
    lv_obj_t *lbl_min_label = label_at(tile_min, "MIN:", &style_label_small, 8, 4);
    lv_obj_set_style_text_color(lbl_min_label, DayColours::GREEN, 0);
    lv_obj_t *lbl_min_val = lv_label_create(tile_min);
    lbl_wmin_value = lbl_min_val;
    lv_obj_add_style(lbl_min_val, &style_val_green, 0);
    apply_glow(lbl_min_val, DayColours::GREEN, DayGlow::GREEN);
    lv_label_set_text_fmt(lbl_min_val, "%.0f", (g_TWS_min < 1e8 ? g_TWS_min : 0));
    lv_obj_align(lbl_min_val, LV_ALIGN_CENTER, -30, 18);
    lv_obj_t *lbl_min_unit = lv_label_create(tile_min);
    lv_obj_add_style(lbl_min_unit, &style_unit, 0);
    lv_label_set_text(lbl_min_unit, "M/S");
    lv_obj_align_to(lbl_min_unit, lbl_min_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 12, -6);
  }

  // Create and display autopilot sheet
  static void buildApSheet() {
    if (ap_sheet) return;
    ap_sheet = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(ap_sheet);
    lv_obj_add_style(ap_sheet, &style_card, 0);
    int sheetH = DISPLAY_HEIGHT * 88 / 100; // 88% height
    lv_obj_set_size(ap_sheet, DISPLAY_WIDTH - PADDING * 2, sheetH);
    lv_obj_set_pos(ap_sheet, PADDING, -sheetH);
    // Header row
    lv_obj_t *row = lv_obj_create(ap_sheet);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), 64);
    lv_obj_set_style_pad_all(row, 12, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 0);
    // Mode chip
    ap_chip_mode = lv_btn_create(row);
    lv_obj_add_style(ap_chip_mode, &style_card, 0);
    lv_obj_set_style_bg_color(ap_chip_mode, DayColours::CARD_DARK, 0);
    lv_obj_set_style_text_color(ap_chip_mode, DayColours::CYAN, 0);
    lv_obj_set_style_pad_all(ap_chip_mode, 10, 0);
    lv_obj_set_size(ap_chip_mode, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(ap_chip_mode, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(ap_chip_mode, [](lv_event_t *e){
      // cycle AP modes
      if (g_apMode == APMode::Heading) g_apMode = APMode::Track;
      else if (g_apMode == APMode::Track) g_apMode = APMode::Wind;
      else g_apMode = APMode::Heading;
      // transmit mode change while keeping engage state and setpoint
      canbus::transmitApCommand(g_apEngaged, g_apMode, g_set_deg);
      updateApSheet();
    }, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(ap_chip_mode), "Steering: Heading");
    // Engage button
    ap_btn_engage = lv_btn_create(row);
    lv_obj_add_style(ap_btn_engage, &style_card, 0);
    lv_obj_set_style_bg_color(ap_btn_engage, DayColours::CARD_DARK, 0);
    lv_obj_set_style_text_color(ap_btn_engage, DayColours::ORANGE, 0);
    lv_obj_set_style_pad_all(ap_btn_engage, 10, 0);
    lv_obj_align(ap_btn_engage, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(ap_btn_engage, [](lv_event_t *e){
      // toggle engage/standby
      bool engage = !g_apEngaged;
      canbus::transmitApCommand(engage, g_apMode, engage ? g_hdg_deg : g_set_deg);
      updateApSheet();
    }, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(ap_btn_engage), "Engage");
    // Main value block
    lv_obj_t *block = lv_obj_create(ap_sheet);
    lv_obj_remove_style_all(block);
    lv_obj_add_style(block, &style_card, 0);
    lv_obj_set_size(block, lv_pct(100), 220);
    lv_obj_align(block, LV_ALIGN_TOP_MID, 0, 72);
    ap_lbl_hdg = lv_label_create(block);
    lv_obj_set_style_text_font(ap_lbl_hdg, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(ap_lbl_hdg, DayColours::WHITE, 0);
    apply_glow(ap_lbl_hdg, DayColours::WHITE, DayGlow::WHITE);
    lv_obj_align(ap_lbl_hdg, LV_ALIGN_LEFT_MID, 24, -20);
    ap_lbl_set = lv_label_create(block);
    lv_obj_set_style_text_font(ap_lbl_set, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(ap_lbl_set, DayColours::CYAN, 0);
    apply_glow(ap_lbl_set, DayColours::CYAN, DayGlow::CYAN);
    lv_obj_align(ap_lbl_set, LV_ALIGN_LEFT_MID, 24, 60);
    ap_lbl_xte = lv_label_create(block);
    lv_obj_set_style_text_font(ap_lbl_xte, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(ap_lbl_xte, DayColours::GREEN, 0);
    lv_obj_align(ap_lbl_xte, LV_ALIGN_RIGHT_MID, -24, 0);
    // Coarse adjust buttons
    static const char *coarse_map[] = {"-10", "-1", "+1", "+10", ""};
    ap_btnm_coarse = lv_btnmatrix_create(ap_sheet);
    lv_btnmatrix_set_map(ap_btnm_coarse, coarse_map);
    lv_obj_set_size(ap_btnm_coarse, lv_pct(100), 80);
    lv_obj_align(ap_btnm_coarse, LV_ALIGN_TOP_MID, 0, 72 + 220 + 16);
    lv_obj_add_event_cb(ap_btnm_coarse, [](lv_event_t *e){
      uint16_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
      if (id == LV_BTNMATRIX_BTN_NONE) return;
      const char *txt = lv_btnmatrix_get_btn_text(lv_event_get_target(e), id);
      if (strcmp(txt, "-10") == 0) canbus::adjustApSetPoint(-10);
      else if (strcmp(txt, "-1") == 0) canbus::adjustApSetPoint(-1);
      else if (strcmp(txt, "+1") == 0) canbus::adjustApSetPoint(+1);
      else if (strcmp(txt, "+10") == 0) canbus::adjustApSetPoint(+10);
      updateApSheet();
    }, LV_EVENT_VALUE_CHANGED, NULL);
    // Fine adjust buttons (tack/advance/gybe)
    static const char *fine_map[] = {"Tack", "Advance WP", "Gybe", ""};
    ap_btnm_fine = lv_btnmatrix_create(ap_sheet);
    lv_btnmatrix_set_map(ap_btnm_fine, fine_map);
    lv_obj_set_size(ap_btnm_fine, lv_pct(100), 80);
    lv_obj_align(ap_btnm_fine, LV_ALIGN_TOP_MID, 0, 72 + 220 + 16 + 80 + 12);
    lv_obj_add_event_cb(ap_btnm_fine, [](lv_event_t *e){
      uint16_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
      if (id == LV_BTNMATRIX_BTN_NONE) return;
      const char *txt = lv_btnmatrix_get_btn_text(lv_event_get_target(e), id);
      if (strcmp(txt, "Tack") == 0) {
        canbus::adjustApSetPoint(+90);
      } else if (strcmp(txt, "Gybe") == 0) {
        canbus::adjustApSetPoint(-90);
      } else if (strcmp(txt, "Advance WP") == 0) {
        // Placeholder: handle waypoint advance if supported
      }
      updateApSheet();
    }, LV_EVENT_VALUE_CHANGED, NULL);
    // Close zone (tap to hide)
    lv_obj_t *close_zone = lv_obj_create(ap_sheet);
    lv_obj_remove_style_all(close_zone);
    lv_obj_set_size(close_zone, lv_pct(100), lv_pct(100));
    lv_obj_align(close_zone, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(close_zone, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(close_zone, [](lv_event_t *e){ apHide(); }, LV_EVENT_CLICKED, NULL);
    updateApSheet();
    // slide in
    lv_anim_t a; lv_anim_init(&a);
    lv_anim_set_var(&a, ap_sheet);
    lv_anim_set_time(&a, 220);
    lv_anim_set_values(&a, -sheetH, PADDING);
    lv_anim_set_exec_cb(&a, [](void *o, int32_t v){ lv_obj_set_y((lv_obj_t*)o, v); });
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
  }

  // Show autopilot sheet
  static void apShow() {
    if (ap_sheet) return;
    buildApSheet();
  }

  // Hide autopilot sheet
  static void apHide() {
    if (!ap_sheet) return;
    int sheetH = lv_obj_get_height(ap_sheet);
    lv_anim_t a; lv_anim_init(&a);
    lv_anim_set_var(&a, ap_sheet);
    lv_anim_set_time(&a, 200);
    lv_anim_set_values(&a, lv_obj_get_y(ap_sheet), -sheetH);
    lv_anim_set_exec_cb(&a, [](void *o, int32_t v){ lv_obj_set_y((lv_obj_t*)o, v); });
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_deleted_cb(&a, [](lv_anim_t *){ if (ap_sheet) { lv_obj_del(ap_sheet); ap_sheet = nullptr; } });
    lv_anim_start(&a);
  }

  // Update autopilot sheet texts according to state
  static void updateApSheet() {
    if (!ap_sheet) return;
    const char *modeStr;
    switch (g_apMode) {
      case APMode::Heading: modeStr = "Steering: Heading"; break;
      case APMode::Track:  modeStr = "Steering: Track"; break;
      case APMode::Wind:   modeStr = "Steering: Wind"; break;
      default:             modeStr = "Standby"; break;
    }
    lv_label_set_text(lv_obj_get_child(ap_chip_mode, 0), modeStr);
    lv_label_set_text(lv_obj_get_child(ap_btn_engage, 0), g_apEngaged ? "Standby" : "Engage");
    lv_label_set_text_fmt(ap_lbl_hdg, "HDG  %.0f°", g_hdg_deg);
    lv_label_set_text_fmt(ap_lbl_set, "SET  %.0f°", g_set_deg);
    lv_label_set_text_fmt(ap_lbl_xte, "XTE  %.1f m", g_xte_m);
  }

  // Build persistent night mode toggle button
  static void buildNightButton() {
    night_btn = lv_btn_create(lv_scr_act());
    lv_obj_add_style(night_btn, &style_card, 0);
    lv_obj_set_style_bg_color(night_btn, DayColours::CARD_DARK, 0);
    lv_obj_set_style_pad_all(night_btn, 12, 0);
    lv_obj_set_pos(night_btn, DISPLAY_WIDTH - 80 - PADDING, DISPLAY_HEIGHT - 80 - PADDING);
    lv_obj_set_size(night_btn, 80, 80);
    lv_obj_add_event_cb(night_btn, [](lv_event_t *e){ toggleNightMode(); }, LV_EVENT_CLICKED, NULL);
    updateNightButton();
  }

  // Update night mode button label/icon
  static void updateNightButton() {
    if (!night_btn) return;
    // Remove children and recreate label with appropriate symbol
    while (lv_obj_get_child_cnt(night_btn) > 0) {
      lv_obj_t *child = lv_obj_get_child(night_btn, 0);
      lv_obj_del(child);
    }
    const char *text = g_nightMode ? LV_SYMBOL_SUN : LV_SYMBOL_MOON;
    lv_obj_t *lbl = lv_label_create(night_btn);
    lv_obj_add_style(lbl, &style_unit, 0);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);
  }

  // Toggle night mode and update styles
  void toggleNightMode() {
    g_nightMode = !g_nightMode;
    // Update colours on styles
    if (g_nightMode) {
      // Set all text colours to dark red
      lv_style_set_text_color(&style_label_small, NightColours::PRIMARY);
      lv_style_set_text_color(&style_unit,       NightColours::PRIMARY);
      lv_style_set_text_color(&style_val_cyan,    NightColours::PRIMARY);
      lv_style_set_text_color(&style_val_orange,  NightColours::PRIMARY);
      lv_style_set_text_color(&style_val_green,   NightColours::PRIMARY);
      lv_style_set_text_color(&style_val_white,   NightColours::PRIMARY);
    } else {
      // Restore day mode colours
      lv_style_set_text_color(&style_label_small, DayColours::WHITE);
      lv_style_set_text_color(&style_unit,       DayColours::WHITE);
      lv_style_set_text_color(&style_val_cyan,    DayColours::CYAN);
      lv_style_set_text_color(&style_val_orange,  DayColours::ORANGE);
      lv_style_set_text_color(&style_val_green,   DayColours::GREEN);
      lv_style_set_text_color(&style_val_white,   DayColours::WHITE);
    }
    // Update glow for each label with dark red if night, else original
    lv_color_t glowCol = g_nightMode ? NightColours::PRIMARY : DayColours::WHITE;
    uint8_t glowOpa = g_nightMode ? NightGlow::PRIMARY : DayGlow::WHITE;
    // A helper lambda to apply glow to a label
    auto refresh_label = [&](lv_obj_t *lbl){ if (!lbl) return; apply_glow(lbl, glowCol, glowOpa); };
    refresh_label(lbl_stw_value);
    refresh_label(lbl_rpm_s_value);
    refresh_label(lbl_gear_value);
    refresh_label(lbl_regen_value);
    refresh_label(lbl_soc_s_value);
    refresh_label(lbl_rpm_o_value);
    refresh_label(lbl_remP_value);
    refresh_label(lbl_batt_value);
    refresh_label(lbl_pdraw_value);
    refresh_label(lbl_soc_o_value);
    refresh_label(lbl_dist_value);
    refresh_label(lbl_ttg_value);
    refresh_label(lbl_ws_center);
    refresh_label(lbl_twa_value);
    refresh_label(lbl_awa_value);
    refresh_label(lbl_wmax_value);
    refresh_label(lbl_wtws_value);
    refresh_label(lbl_waws_value);
    refresh_label(lbl_wmin_value);
    refresh_label(ap_lbl_hdg);
    refresh_label(ap_lbl_set);
    refresh_label(ap_lbl_xte);
    // update night button icon
    updateNightButton();
    // Refresh the screen
    lv_obj_report_style_change(NULL);
  }

  // Periodic UI update
  void tick() {
    // Speed page
    lv_label_set_text_fmt(lbl_stw_value, "%.1f", g_STW_kts);
    lv_label_set_text_fmt(lbl_rpm_s_value, "%d", g_RPM);
    lv_label_set_text_fmt(lbl_gear_value, "%c", g_Gear);
    if (fabs(g_RegenA) < 0.05f) lv_label_set_text(lbl_regen_value, "-");
    else lv_label_set_text_fmt(lbl_regen_value, "%.0f A", fabs(g_RegenA));
    lv_label_set_text_fmt(lbl_soc_s_value, "%d %%", g_SOC_pct);
    // Overview page
    lv_label_set_text_fmt(lbl_rpm_o_value, "%d", g_RPM);
    lv_label_set_text_fmt(lbl_remP_value, "%.1f", g_Rem_kWh);
    lv_label_set_text_fmt(lbl_batt_value, "%.0f", g_Batt_V);
    lv_label_set_text_fmt(lbl_pdraw_value, "%.1f", g_Pdraw_kW);
    lv_label_set_text_fmt(lbl_soc_o_value, "%d", g_SOC2_pct);
    lv_label_set_text_fmt(lbl_dist_value, "%d", g_Dist_kts);
    lv_label_set_text_fmt(lbl_ttg_value, "%d", g_TTG_hrs);
    // Wind page
    lv_label_set_text_fmt(lbl_ws_center, "%.0f\nM/S", g_TWS_ms);
    lv_label_set_text_fmt(lbl_twa_value, "%.0f", g_TWA_deg);
    lv_label_set_text_fmt(lbl_awa_value, "%.0f", g_AWA_deg);
    lv_label_set_text_fmt(lbl_wmax_value, "%.0f", (g_TWS_max > 0 ? g_TWS_max : 0));
    lv_label_set_text_fmt(lbl_wtws_value, "%.0f", g_TWS_ms);
    lv_label_set_text_fmt(lbl_waws_value, "%.0f", g_AWS_ms);
    lv_label_set_text_fmt(lbl_wmin_value, "%.0f", (g_TWS_min < 1e8 ? g_TWS_min : 0));
    // update wind needle (true wind angle)
    if (wind_needle && wind_meter) {
      lv_meter_set_indicator_value(wind_meter, wind_needle, (int)g_TWA_deg);
    }
    // Autopilot sheet values if open
    updateApSheet();
  }

} // namespace ui
