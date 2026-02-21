/**
 * Clock / Screensaver Feature
 * NTP clock with optional loremflickr.com photo background
 * Declarations, timezone list, and state management
 */

#ifndef CLOCK_SCREEN_H
#define CLOCK_SCREEN_H

#include <Arduino.h>
#include "lvgl.h"
#include "config.h"

// ============================================================================
// Timezone List (82 zones — IANA name + POSIX TZ string)
// ============================================================================
struct ClockZone {
    const char* name;
    const char* posix;
};

static const ClockZone CLOCK_ZONES[] = {
    // UTC
    {"UTC",                              "UTC0"},
    // Africa
    {"Africa/Abidjan",                   "GMT0"},
    {"Africa/Addis_Ababa",               "EAT-3"},
    {"Africa/Cairo",                     "EET-2"},
    {"Africa/Casablanca",                "WET0"},
    {"Africa/Johannesburg",              "SAST-2"},
    {"Africa/Lagos",                     "WAT-1"},
    {"Africa/Nairobi",                   "EAT-3"},
    // Americas
    {"America/Adak",                     "HAST10HADT,M3.2.0,M11.1.0"},
    {"America/Anchorage",                "AKST9AKDT,M3.2.0,M11.1.0"},
    {"America/Argentina/Buenos_Aires",   "ART3"},
    {"America/Bogota",                   "COT5"},
    {"America/Caracas",                  "VET4:30"},
    {"America/Chicago",                  "CST6CDT,M3.2.0,M11.1.0"},
    {"America/Denver",                   "MST7MDT,M3.2.0,M11.1.0"},
    {"America/Halifax",                  "AST4ADT,M3.2.0,M11.1.0"},
    {"America/Lima",                     "PET5"},
    {"America/Los_Angeles",              "PST8PDT,M3.2.0,M11.1.0"},
    {"America/Mexico_City",              "CST6CDT,M4.1.0,M10.5.0"},
    {"America/New_York",                 "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Phoenix",                  "MST7"},
    {"America/Santiago",                 "CLT4CLST,M10.2.6/24,M3.2.6/24"},
    {"America/Sao_Paulo",                "BRT3BRST,M10.3.0/0,M2.3.0/0"},
    {"America/St_Johns",                 "NST3:30NDT,M3.2.0,M11.1.0"},
    {"America/Toronto",                  "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Vancouver",                "PST8PDT,M3.2.0,M11.1.0"},
    // Asia
    {"Asia/Baghdad",                     "AST-3"},
    {"Asia/Bangkok",                     "ICT-7"},
    {"Asia/Colombo",                     "IST-5:30"},
    {"Asia/Dhaka",                       "BDT-6"},
    {"Asia/Dubai",                       "GST-4"},
    {"Asia/Ho_Chi_Minh",                 "ICT-7"},
    {"Asia/Hong_Kong",                   "HKT-8"},
    {"Asia/Jakarta",                     "WIB-7"},
    {"Asia/Karachi",                     "PKT-5"},
    {"Asia/Kathmandu",                   "NPT-5:45"},
    {"Asia/Kolkata",                     "IST-5:30"},
    {"Asia/Kuala_Lumpur",                "MYT-8"},
    {"Asia/Kuwait",                      "AST-3"},
    {"Asia/Manila",                      "PST-8"},
    {"Asia/Riyadh",                      "AST-3"},
    {"Asia/Seoul",                       "KST-9"},
    {"Asia/Shanghai",                    "CST-8"},
    {"Asia/Singapore",                   "SGT-8"},
    {"Asia/Taipei",                      "CST-8"},
    {"Asia/Tehran",                      "IRST-3:30IRDT,80/0,264/0"},
    {"Asia/Tokyo",                       "JST-9"},
    {"Asia/Yekaterinburg",               "YEKT-5"},
    // Atlantic
    {"Atlantic/Azores",                  "AZOT1AZOST,M3.5.0/0,M10.5.0/1"},
    {"Atlantic/Reykjavik",               "GMT0"},
    // Australia
    {"Australia/Adelaide",               "ACST-9:30ACDT,M10.1.0,M4.1.0/3"},
    {"Australia/Brisbane",               "AEST-10"},
    {"Australia/Darwin",                 "ACST-9:30"},
    {"Australia/Melbourne",              "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"Australia/Perth",                  "AWST-8"},
    {"Australia/Sydney",                 "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    // Europe
    {"Europe/Amsterdam",                 "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Athens",                    "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Berlin",                    "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Brussels",                  "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Bucharest",                 "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Budapest",                  "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Copenhagen",                "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Dublin",                    "GMT0IST,M3.5.0/1,M10.5.0"},
    {"Europe/Helsinki",                  "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Istanbul",                  "TRT-3"},
    {"Europe/Kiev",                      "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Lisbon",                    "WET0WEST,M3.5.0/1,M10.5.0"},
    {"Europe/London",                    "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Europe/Madrid",                    "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Moscow",                    "MSK-3"},
    {"Europe/Oslo",                      "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Paris",                     "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Prague",                    "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Rome",                      "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Stockholm",                 "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Tallinn",                   "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Vienna",                    "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Warsaw",                    "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Zurich",                    "CET-1CEST,M3.5.0,M10.5.0/3"},
    // Pacific
    {"Pacific/Auckland",                 "NZST-12NZDT,M9.5.0,M4.1.0/3"},
    {"Pacific/Fiji",                     "FJT-12"},
    {"Pacific/Guam",                     "ChST-10"},
    {"Pacific/Honolulu",                 "HST10"},
    {"Pacific/Port_Moresby",             "PGT-10"},
};

static const int CLOCK_ZONES_COUNT = (int)(sizeof(CLOCK_ZONES) / sizeof(CLOCK_ZONES[0]));

// ============================================================================
// Photo background keyword list (used by settings dropdown + clockBgTask URL)
// ============================================================================
struct ClockBgKeyword {
    const char* label;  // Shown in settings dropdown
    const char* kw;     // Appended to loremflickr URL ("" = no keyword = truly random)
};

static const ClockBgKeyword CLOCK_BG_KEYWORDS[] = {
    {"Random",        ""},
    {"Landscape",     "landscape"},
    {"Nature",        "nature"},
    {"City",          "city"},
    {"Architecture",  "architecture"},
    {"Ocean",         "ocean"},
    {"Mountain",      "mountain"},
    {"Forest",        "forest"},
    {"Sunset",        "sunset"},
    {"Abstract",      "abstract"},
    {"Travel",        "travel"},
    {"Space",         "space"},
};
static const int CLOCK_BG_KW_COUNT = (int)(sizeof(CLOCK_BG_KEYWORDS) / sizeof(CLOCK_BG_KEYWORDS[0]));

// ============================================================================
// Clock State Machine
// ============================================================================
enum ClockState {
    CLOCK_IDLE,       // Normal operation — monitoring for trigger conditions
    CLOCK_ENTERING,   // Waiting for art/lyrics tasks to stop before showing clock
    CLOCK_ACTIVE,     // Clock screen is displayed
    CLOCK_EXITING,    // Clock dismissed — waiting for bg task to stop before cleanup
};

// ============================================================================
// Clock Settings (loaded from NVS on boot)
// ============================================================================
extern int  clock_mode;           // CLOCK_MODE_* enum value
extern int  clock_timeout_min;    // Minutes of inactivity before clock appears
extern int  clock_tz_idx;         // Index into CLOCK_ZONES[]
extern bool clock_picsum_enabled; // true = download random photo background
extern int  clock_refresh_min;    // Minutes between background photo refreshes
extern int  clock_bg_kw_idx;      // Index into CLOCK_BG_KEYWORDS[]
extern bool clock_12h;            // true = 12h AM/PM format, false = 24h

// ============================================================================
// Clock Runtime State
// ============================================================================
extern ClockState clock_state;
extern uint32_t   clock_entering_start_ms;   // When CLOCK_ENTERING began
extern uint32_t   clock_exiting_start_ms;    // When CLOCK_EXITING began
extern uint32_t   last_clock_exit_ms;        // Last time clock was dismissed

// ============================================================================
// Clock Background Task
// ============================================================================
extern TaskHandle_t clockBgTaskHandle;
extern volatile bool clock_bg_shutdown_requested;
extern volatile bool clock_bg_ready;     // New image decoded and ready to display
extern uint16_t*     clock_bg_buffer;    // PSRAM pixel buffer (800×480 RGB565)
extern lv_img_dsc_t  clock_bg_dsc;      // LVGL image descriptor pointing to buffer

// ============================================================================
// Clock Screen UI Objects (set during createClockScreen)
// ============================================================================
extern lv_obj_t* scr_clock;
extern lv_obj_t* scr_clock_settings;
extern lv_obj_t* clock_bg_img;      // Background image widget
extern lv_obj_t* clock_time_lbl;    // HH:MM:SS label
extern lv_obj_t* clock_date_lbl;    // Day, Month DD YYYY label

// ============================================================================
// Function Declarations
// ============================================================================
void createClockScreen();
void createClockSettingsScreen();
void checkClockTrigger();
void exitClockScreen();
void clockBgTask(void* param);

#endif // CLOCK_SCREEN_H
