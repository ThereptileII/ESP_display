/*
 * state.cpp
 *
 * Defines the global variables declared in state.h.
 */

#include "state.h"

bool g_nightMode = false;

float g_STW_kts  = 0.0f;
int   g_RPM      = 0;
char  g_Gear     = 'N';
float g_RegenA   = 0.0f;
int   g_SOC_pct  = 0;
float g_Rem_kWh  = 0.0f;
float g_Batt_V   = 0.0f;
int   g_SOC2_pct = 0;
float g_Pdraw_kW = 0.0f;
int   g_Dist_kts = 0;
int   g_TTG_hrs  = 0;

float g_AWS_ms   = 0.0f;
float g_TWS_ms   = 0.0f;
float g_AWA_deg  = 0.0f;
float g_TWA_deg  = 0.0f;
float g_TWS_max  = -1e9f;
float g_TWS_min  =  1e9f;

APMode g_apMode     = APMode::Standby;
bool   g_apEngaged  = false;
float  g_hdg_deg    = 0.0f;
float  g_set_deg    = 0.0f;
float  g_xte_m      = 0.0f;
float  g_rudder_deg = 0.0f;
