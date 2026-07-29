#ifndef RTC_H
#define RTC_H

#include <Arduino.h>
#include "Globals.h"

// ============================================================================
// TIME STRUCTURE
// ============================================================================

struct TimeData {
  uint8_t year;      // 0-99 (2000-2099)
  uint8_t month;     // 1-12
  uint8_t day;       // 1-31
  uint8_t hour;      // 0-23
  uint8_t minute;    // 0-59
  uint8_t second;    // 0-59
  uint8_t dayOfWeek; // 0-6 (0=Sunday)
};

// ============================================================================
// RTC FUNCTIONS
// ============================================================================

/**
 * Initialize RTC module
 * NOTE: Requires DS3231 or similar RTC on I2C bus
 */
void rtc_init();

/**
 * Update RTC and internal timing
 * Should be called periodically
 */
void rtc_update();

/**
 * Get current time
 * @return - pointer to TimeData structure
 */
TimeData* rtc_getTime();

/**
 * Set time on RTC module
 * @param time - TimeData to set
 * @return - true if successful
 */
bool rtc_setTime(TimeData* time);

/**
 * Get current Unix timestamp
 * @return - seconds since 2000-01-01 00:00:00
 */
unsigned long rtc_getUnixTime();

/**
 * Check if RTC has valid time
 * @return - true if RTC has been set and is running
 */
bool rtc_isValid();

/**
 * Get day name
 * @param dayOfWeek - 0-6 (0=Sunday)
 * @return - day name string
 */
const char* rtc_getDayName(uint8_t dayOfWeek);

/**
 * Get month name
 * @param month - 1-12
 * @return - month name string
 */
const char* rtc_getMonthName(uint8_t month);

#endif // RTC_H
