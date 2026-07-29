#include "RTC.h"
#include <Wire.h>

// ============================================================================
// RTC DATA AND CONSTANTS
// ============================================================================

static TimeData currentTime;
static bool rtcValid = false;
const uint8_t DS3231_ADDRESS = 0x68;

// ============================================================================
// INITIALIZATION
// ============================================================================

void rtc_init() {
  // RTC initialization would require DS3231 or similar module
  // For now, initialize with default values
  currentTime.year = 0;
  currentTime.month = 1;
  currentTime.day = 1;
  currentTime.hour = 0;
  currentTime.minute = 0;
  currentTime.second = 0;
  currentTime.dayOfWeek = 0;
  rtcValid = false;

  // TODO: Implement DS3231 I2C communication when hardware available
}

// ============================================================================
// TIME MANAGEMENT
// ============================================================================

void rtc_update() {
  // TODO: Read from DS3231 RTC module via I2C
  // For now, update based on millis() since boot
  static unsigned long lastUpdateMs = 0;
  unsigned long now = millis();

  if (now - lastUpdateMs >= 1000) {
    lastUpdateMs = now;
    currentTime.second++;

    // Handle rollover
    if (currentTime.second >= 60) {
      currentTime.second = 0;
      currentTime.minute++;

      if (currentTime.minute >= 60) {
        currentTime.minute = 0;
        currentTime.hour++;

        if (currentTime.hour >= 24) {
          currentTime.hour = 0;
          currentTime.day++;
          // TODO: Handle month/year rollover
        }
      }
    }
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

  // TODO: Write to DS3231 module
  return true;
}

unsigned long rtc_getUnixTime() {
  // Simplified: return seconds since boot
  // Real implementation would calculate from TimeData
  return millis() / 1000;
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
