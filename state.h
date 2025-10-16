/*
 * state.h
 *
 * This header declares global variables used throughout the dashboard.
 * Variables are initialised with sensible defaults and updated by the
 * canbus and logging modules.  They are consumed by the UI for
 * display.  Night mode is tracked by a boolean flag.
 */

#pragma once

#include <stdint.h>

// Night mode flag – when true, dark red colours are used
extern bool g_nightMode;

// Boat speed (STW) in knots
extern float g_STW_kts;

// Engine RPM
extern int g_RPM;

// Gear selection: 'D', 'N', 'R' (drive, neutral, reverse)
extern char g_Gear;

// Regeneration current in amps (negative if charging).  A zero value
// indicates that regeneration is off.
extern float g_RegenA;

// State of charge as a percentage (primary display)
extern int g_SOC_pct;

// Remaining energy in kWh
extern float g_Rem_kWh;

// Battery voltage in volts
extern float g_Batt_V;

// Battery state of charge on overview page (may duplicate g_SOC_pct)
extern int g_SOC2_pct;

// Power draw in kW
extern float g_Pdraw_kW;

// Distance to go (units determined by context, e.g. nautical miles or knots; this
// value is a placeholder from the design)
extern int g_Dist_kts;

// Time to go in hours
extern int g_TTG_hrs;

// True wind and apparent wind data (m/s and degrees)
extern float g_AWS_ms, g_TWS_ms;
extern float g_AWA_deg, g_TWA_deg;

// Session maximum and minimum TWS values
extern float g_TWS_max, g_TWS_min;

// Autopilot state
enum class APMode { Standby=0, Heading=1, Track=2, Wind=3 };
extern APMode g_apMode;
extern bool g_apEngaged;
extern float g_hdg_deg;       // vessel heading
extern float g_set_deg;       // autopilot setpoint
extern float g_xte_m;         // cross track error (m)
extern float g_rudder_deg;    // rudder angle
