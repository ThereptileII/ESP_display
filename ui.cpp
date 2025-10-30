#include "ui.h"
#include "config.h"

#if ORIENTATION_MODE==1 || ORIENTATION_MODE==2
static const int SCREEN_W=1280, SCREEN_H=800;
#else
static const int SCREEN_W=800, SCREEN_H=1280;
#endif

#if defined(__has_include)
  #if __has_include("fonts/orbitron_48_900.c")
    extern "C" {
      extern const lv_font_t orbitron_48_900;
      extern const lv_font_t orbitron_32_800;
      extern const lv_font_t orbitron_20_700;
      extern const lv_font_t orbitron_16_600;
    }
    #define HAVE_ORBITRON 1
  #endif
#endif

#ifndef HAVE_ORBITRON
  extern "C" {
    extern const lv_font_t lv_font_montserrat_48;
    extern const lv_font_t lv_font_montserrat_32;
    extern const lv_font_t lv_font_montserrat_20;
    extern const lv_font_t lv_font_montserrat_16;
  }
#endif

static lv_style_t st_screen, st_card, st_label, st_val_lg, st_val_md, st_unit, st_label_glow, st_night_text;
static lv_obj_t* root;
static lv_obj_t* pages_cont;
static int page_idx = 0;
static const int PAGE_COUNT = 3;
static lv_obj_t* batt_detail;
static lv_obj_t* batt_detail_back;
static bool night_mode = false;
static lv_obj_t* ap_overlay;
static lv_obj_t* ap_close_btn;

static lv_obj_t *rpm_val, *power_val, *batt_v_val;
static lv_obj_t *wind_spd_val, *wind_ang_val;

static inline lv_color_t HEXC(uint32_t hex) { return lv_color_hex(hex); }

static void apply_styles() {
  lv_style_init(&st_screen);
  lv_style_set_bg_opa(&st_screen, LV_OPA_COVER);
  lv_style_set_bg_color(&st_screen, HEXC(CLR_BG_MAIN));

  lv_style_init(&st_card);
  lv_style_set_radius(&st_card, 16);
  lv_style_set_bg_opa(&st_card, LV_OPA_COVER);
  lv_style_set_bg_color(&st_card, HEXC(CLR_BG_CARD_A));
  lv_style_set_shadow_width(&st_card, 6);
  lv_style_set_shadow_opa(&st_card, LV_OPA_20);
  lv_style_set_shadow_color(&st_card, HEXC(0x000000));

  lv_style_init(&st_label);
#ifdef HAVE_ORBITRON
  lv_style_set_text_font(&st_label, &orbitron_16_600);
#else
  lv_style_set_text_font(&st_label, &lv_font_montserrat_16);
#endif
  lv_style_set_text_color(&st_label, HEXC(CLR_NEARWHITE));
  lv_style_set_text_opa(&st_label, LV_OPA_COVER);

  lv_style_init(&st_label_glow);
  lv_style_set_text_opa(&st_label_glow, LV_OPA_COVER);
  lv_style_set_text_color(&st_label_glow, HEXC(CLR_NEARWHITE));

  lv_style_init(&st_val_lg);
#ifdef HAVE_ORBITRON
  lv_style_set_text_font(&st_val_lg, &orbitron_48_900);
#else
  lv_style_set_text_font(&st_val_lg, &lv_font_montserrat_48);
#endif
  lv_style_set_text_color(&st_val_lg, HEXC(CLR_CYAN));

  lv_style_init(&st_val_md);
#ifdef HAVE_ORBITRON
  lv_style_set_text_font(&st_val_md, &orbitron_32_800);
#else
  lv_style_set_text_font(&st_val_md, &lv_font_montserrat_32);
#endif
  lv_style_set_text_color(&st_val_md, HEXC(CLR_GREEN));

  lv_style_init(&st_unit);
#ifdef HAVE_ORBITRON
  lv_style_set_text_font(&st_unit, &orbitron_20_700);
#else
  lv_style_set_text_font(&st_unit, &lv_font_montserrat_20);
#endif
  lv_style_set_text_color(&st_unit, HEXC(CLR_NEARWHITE));

  lv_style_init(&st_night_text);
  lv_style_set_text_color(&st_night_text, HEXC(CLR_NIGHT_RED));
}

static lv_obj_t* make_tile(lv_obj_t* parent, int x, int y, int w, int h) {
  lv_obj_t* card = lv_obj_create(parent);
  lv_obj_remove_style_all(card);
  lv_obj_add_style(card, &st_card, 0);
  lv_obj_set_pos(card, x, y);
  lv_obj_set_size(card, w, h);
  return card;
}

static lv_obj_t* mk_label(lv_obj_t* parent, const char* txt, lv_style_t* st, lv_coord_t x, lv_coord_t y) {
  lv_obj_t* l = lv_label_create(parent);
  lv_obj_add_style(l, st, 0);
  lv_label_set_text(l, txt);
  lv_obj_set_pos(l, x, y);
  return l;
}

static lv_obj_t* build_page_rpm(lv_obj_t* parent) {
  lv_obj_t* page = lv_obj_create(parent);
  lv_obj_remove_style_all(page);
  lv_obj_add_style(page, &st_screen, 0);
  lv_obj_set_size(page, SCREEN_W, SCREEN_H);

  auto rpm = make_tile(page, 24, 24, SCREEN_W-48, (SCREEN_H-72)/2);
  mk_label(rpm, "RPM:", &st_label, 24, 18);
  rpm_val = mk_label(rpm, "600", &st_val_lg, 24, 90);

  auto pwr = make_tile(page, 24, 24 + (SCREEN_H-72)/2 + 24, (SCREEN_W/2)-36, ((SCREEN_H-72)/2) - 24);
  mk_label(pwr, "Power:", &st_label, 24, 18);
  power_val = mk_label(pwr, "23.2", &st_val_md, 24, 90);
  lv_obj_set_style_text_color(power_val, HEXC(CLR_ORANGE), 0);
  mk_label(pwr, "kW", &st_unit, 220, 100);

  auto bat = make_tile(page, 24 + (SCREEN_W/2), 24 + (SCREEN_H-72)/2 + 24, (SCREEN_W/2)-36, ((SCREEN_H-72)/2) - 24);
  mk_label(bat, "Battery", &st_label, 18, 12);
  batt_v_val = mk_label(bat, "380", &st_val_md, 18, 64);
  lv_obj_set_style_text_color(batt_v_val, HEXC(CLR_GREEN), 0);
  mk_label(bat, "V", &st_unit, 120, 72);
  lv_obj_add_flag(bat, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(bat, [](lv_event_t* e){ ui_open_battery_detail(); }, LV_EVENT_CLICKED, nullptr);

  return page;
}

static lv_obj_t* build_page_battery(lv_obj_t* parent) {
  lv_obj_t* page = lv_obj_create(parent);
  lv_obj_remove_style_all(page);
  lv_obj_add_style(page, &st_screen, 0);
  lv_obj_set_size(page, SCREEN_W, SCREEN_H);

  auto card = make_tile(page, 24, 24, SCREEN_W-48, SCREEN_H-48);
  mk_label(card, "Battery voltage", &st_label, 24, 18);
  mk_label(card, "(1h / 6h / 24h series active; SD logging on)", &st_label, 24, 60);
  return page;
}

static lv_obj_t* build_page_wind(lv_obj_t* parent) {
  lv_obj_t* page = lv_obj_create(parent);
  lv_obj_remove_style_all(page);
  lv_obj_add_style(page, &st_screen, 0);
  lv_obj_set_size(page, SCREEN_W, SCREEN_H);

  auto w = make_tile(page, 24, 24, SCREEN_W-48, (SCREEN_H-72)/2);
  mk_label(w, "Wind:", &st_label, 24, 18);
  wind_spd_val = mk_label(w, "8.4", &st_val_md, 24, 90);
  mk_label(w, "m/s", &st_unit, 140, 100);

  auto a = make_tile(page, 24, 24 + (SCREEN_H-72)/2 + 24, SCREEN_W-48, ((SCREEN_H-72)/2) - 24);
  mk_label(a, "Angle:", &st_label, 24, 18);
  wind_ang_val = mk_label(a, "35°", &st_val_md, 24, 90);

  return page;
}

static void build_battery_detail() {
  batt_detail = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(batt_detail);
  lv_obj_set_size(batt_detail, SCREEN_W, SCREEN_H);
  lv_obj_set_style_bg_opa(batt_detail, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(batt_detail, HEXC(0x0A0D13), 0);
  lv_obj_add_flag(batt_detail, LV_OBJ_FLAG_HIDDEN);

  batt_detail_back = mk_label(batt_detail, "← Back", &st_label, 24, 24);
  lv_obj_add_flag(batt_detail_back, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(batt_detail_back, [](lv_event_t* e){ ui_close_battery_detail(); }, LV_EVENT_CLICKED, nullptr);

  auto card = lv_obj_create(batt_detail);
  lv_obj_remove_style_all(card);
  lv_obj_add_style(card, &st_card, 0);
  lv_obj_set_pos(card, 24, 90);
  lv_obj_set_size(card, SCREEN_W-48, SCREEN_H-120);
  mk_label(card, "Battery detail (tap Back to return)", &st_label, 24, 18);
}

static void build_ap_overlay() {
  ap_overlay = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(ap_overlay);
  lv_obj_set_size(ap_overlay, SCREEN_W, SCREEN_H/1.5);
  lv_obj_set_style_bg_opa(ap_overlay, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(ap_overlay, HEXC(0x0F1218), 0);
  lv_obj_add_flag(ap_overlay, LV_OBJ_FLAG_HIDDEN);

  mk_label(ap_overlay, "Autopilot", &st_label, 24, 18);
  ap_close_btn = mk_label(ap_overlay, "X", &st_label, SCREEN_W-50, 18);
  lv_obj_add_flag(ap_close_btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(ap_close_btn, [](lv_event_t* e){ ui_ap_close(); }, LV_EVENT_CLICKED, nullptr);
}

static lv_point_t touch_start;
static uint32_t   touch_start_ms;
static void on_touch(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    lv_indev_t* indev = lv_indev_get_act();
    lv_indev_get_point(indev, &touch_start);
    touch_start_ms = millis();
  } else if (code == LV_EVENT_RELEASED) {
    lv_indev_t* indev = lv_indev_get_act();
    lv_point_t now; lv_indev_get_point(indev, &now);
    int16_t dx = now.x - touch_start.x;
    int16_t dy = now.y - touch_start.y;
    uint32_t dt = millis() - touch_start_ms;

    const int SWIPE = 60;
    if (abs(dx) > abs(dy) && abs(dx) > SWIPE) {
      if (dx < 0) ui_next_page();
      else ui_prev_page();
    } else if (dy > SWIPE && dt < 1200) {
      ui_ap_open();
    }
  }
}

void ui_set_night_mode(bool enabled) {
  night_mode = enabled;
  if (enabled) lv_obj_add_style(lv_scr_act(), &st_night_text, 0);
  else         lv_obj_remove_style(lv_scr_act(), &st_night_text, 0);
}

void ui_next_page() { if (page_idx < PAGE_COUNT - 1) page_idx++; lv_obj_scroll_to_x(pages_cont, page_idx * SCREEN_W, LV_ANIM_ON); }
void ui_prev_page() { if (page_idx > 0) page_idx--; lv_obj_scroll_to_x(pages_cont, page_idx * SCREEN_W, LV_ANIM_ON); }

lv_obj_t* ui_build() {
  apply_styles();

  root = lv_scr_act();

  lv_obj_t* btn = lv_btn_create(root);
  lv_obj_set_size(btn, 60, 60);
  lv_obj_set_pos(btn, SCREEN_W-80, 18);
  lv_obj_add_event_cb(btn, [](lv_event_t* e){ ui_set_night_mode(!night_mode); }, LV_EVENT_CLICKED, nullptr);
  auto lbl = lv_label_create(btn);
  lv_label_set_text(lbl, "🌙");
  lv_obj_center(lbl);

  pages_cont = lv_obj_create(root);
  lv_obj_remove_style_all(pages_cont);
  lv_obj_set_size(pages_cont, SCREEN_W, SCREEN_H);
  lv_obj_set_style_bg_opa(pages_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_scroll_dir(pages_cont, LV_DIR_HOR);
  lv_obj_set_scrollbar_mode(pages_cont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_flex_flow(pages_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_scroll_snap_x(pages_cont, LV_SCROLL_SNAP_CENTER);
  lv_obj_add_event_cb(pages_cont, on_touch, LV_EVENT_ALL, nullptr);

  (void)build_page_rpm(pages_cont);
  (void)build_page_battery(pages_cont);
  (void)build_page_wind(pages_cont);

  build_battery_detail();
  build_ap_overlay();

  lv_obj_scroll_to_x(pages_cont, 0, LV_ANIM_OFF);
  return root;
}

void ui_update_rpm(uint16_t rpm) {
  if (rpm_val) { char buf[16]; snprintf(buf, sizeof(buf), "%u", (unsigned)rpm); lv_label_set_text(rpm_val, buf); }
}
void ui_update_power_kw(float kw) {
  if (power_val) { char buf[24]; snprintf(buf, sizeof(buf), "%.1f", kw); lv_label_set_text(power_val, buf); }
}
void ui_update_batt_v(float v) {
  if (batt_v_val) { char buf[24]; snprintf(buf, sizeof(buf), "%.1f", v); lv_label_set_text(batt_v_val, buf); }
}
void ui_update_wind(float speed_ms, float angle_rad) {
  if (wind_spd_val) { char b1[24]; snprintf(b1, sizeof(b1), "%.1f", speed_ms); lv_label_set_text(wind_spd_val, b1); }
  if (wind_ang_val) { float deg = angle_rad * 57.2957795f; char b2[24]; snprintf(b2, sizeof(b2), "%.0f°", deg); lv_label_set_text(wind_ang_val, b2); }
}

void ui_open_battery_detail() { lv_obj_clear_flag(batt_detail, LV_OBJ_FLAG_HIDDEN); }
void ui_close_battery_detail(){ lv_obj_add_flag(batt_detail,   LV_OBJ_FLAG_HIDDEN); }
void ui_ap_open()  { lv_obj_clear_flag(ap_overlay, LV_OBJ_FLAG_HIDDEN); }
void ui_ap_close() { lv_obj_add_flag(ap_overlay,   LV_OBJ_FLAG_HIDDEN); }
void ui_tick() {}
