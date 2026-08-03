#include "RTC.h"
#include <time.h>

// ============================================================================
// NTP CONFIGURATION
// ============================================================================

static const char* NTP_SERVER_1  = "pool.ntp.org";
static const char* NTP_SERVER_2  = "time.nist.gov";
static const long  GMT_OFFSET_SEC  = 3600;   // UTC+1 (CET); change to 7200 for CEST
static const int   DAYLIGHT_OFFSET_SEC = 3600;

// ============================================================================
// RTC DATA
// ============================================================================

static TimeData currentTime;
static bool rtcValid = false;

// ============================================================================
// HELPERS
// ============================================================================

static uint8_t dayOfWeekFromTm(int wday) {
  // struct tm: 0=Sunday … 6=Saturday — same as TimeData convention
  return (uint8_t)wday;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void rtc_init() {
  currentTime.year      = 0;
  currentTime.month     = 1;
  currentTime.day       = 1;
  currentTime.hour      = 0;
  currentTime.minute    = 0;
  currentTime.second    = 0;
  currentTime.dayOfWeek = 0;
  rtcValid = false;

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2);
}

void rtc_syncNTP() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    currentTime.year      = (uint8_t)(timeinfo.tm_year - 100); // years since 1900 → offset from 2000
    currentTime.month     = (uint8_t)(timeinfo.tm_mon + 1);    // 0-based → 1-based
    currentTime.day       = (uint8_t)timeinfo.tm_mday;
    currentTime.hour      = (uint8_t)timeinfo.tm_hour;
    currentTime.minute    = (uint8_t)timeinfo.tm_min;
    currentTime.second    = (uint8_t)timeinfo.tm_sec;
    currentTime.dayOfWeek = dayOfWeekFromTm(timeinfo.tm_wday);
    rtcValid = true;
  }
}

// ============================================================================
// TIME MANAGEMENT
// ============================================================================

void rtc_update() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    currentTime.year      = (uint8_t)(timeinfo.tm_year - 100);
    currentTime.month     = (uint8_t)(timeinfo.tm_mon + 1);
    currentTime.day       = (uint8_t)timeinfo.tm_mday;
    currentTime.hour      = (uint8_t)timeinfo.tm_hour;
    currentTime.minute    = (uint8_t)timeinfo.tm_min;
    currentTime.second    = (uint8_t)timeinfo.tm_sec;
    currentTime.dayOfWeek = dayOfWeekFromTm(timeinfo.tm_wday);
    rtcValid = true;
  }
}

// ============================================================================
// PUBLIC API
// ============================================================================

TimeData* rtc_getTime() {
  return &currentTime;
}

bool rtc_setTime(TimeData* time) {
  if (time == nullptr) {
    return false;
  }
  currentTime = *time;
  rtcValid = true;
  return true;
}

unsigned long rtc_getUnixTime() {
  return (unsigned long)::time(nullptr);
}

bool rtc_isValid() {
  return rtcValid;
}

const char* rtc_getDayName(uint8_t dayOfWeek) {
  const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                        "Thursday", "Friday", "Saturday"};
  if (dayOfWeek > 6) {
    return "Unknown";
  }
  return days[dayOfWeek];
}

const char* rtc_getMonthName(uint8_t month) {
  const char* months[] = {"Invalid", "January", "February", "March",
                          "April", "May", "June", "July", "August",
                          "September", "October", "November", "December"};
  if (month > 12) {
    return "Invalid";
  }
  return months[month];
}
