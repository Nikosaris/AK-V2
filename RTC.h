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
// TIME SOURCE ENUM
// ============================================================================

enum class RTCSource : uint8_t {
  MILLIS  = 0,  // Software fallback, time kept by millis()
  DS3231  = 1,  // Hardware DS3231 RTC module (I2C)
  NTP     = 2   // NTP synchronised (written back to DS3231 when available)
};

// ============================================================================
// RTC FUNCTIONS
// ============================================================================

/**
 * Initialize RTC module.
 * Tries DS3231 first; falls back to MILLIS if not present.
 */
void rtc_init();

/**
 * Update RTC and internal timing.
 * Should be called every loop iteration.
 */
void rtc_update();

/**
 * Get current time.
 * @return pointer to TimeData structure
 */
TimeData* rtc_getTime();

/**
 * Set time (also writes to DS3231 when available).
 * Marks time as valid.
 * @param time - TimeData to set
 * @return true if successful
 */
bool rtc_setTime(TimeData* time);

/**
 * Synchronise time from an NTP Unix timestamp (seconds since 1970-01-01).
 * Converts to TimeData, marks source as NTP, and writes to DS3231 if present.
 * @param unixTime - epoch seconds (UTC)
 * @param timezoneOffsetSeconds - signed offset to apply (e.g. +3600 for UTC+1)
 * @return true if successful
 */
bool rtc_syncFromNTP(unsigned long unixTime, int32_t timezoneOffsetSeconds);

/**
 * Synchronise time directly from a TimeData struct (e.g. from EthernetNTP).
 * Marks source as NTP and writes to DS3231 if present.
 * @param time - pointer to TimeData with the new time
 * @return true if successful
 */
bool rtc_syncFromNTP(TimeData* time);

/**
 * Get current time source.
 */
RTCSource rtc_getSource();

/**
 * Alias for rtc_getSource() — used by AK_V2.ino logSystemStatus().
 */
RTCSource rtc_getCurrentSource();

/**
 * Get human-readable name for a time source.
 * @param source - RTCSource value
 * @return source name string (e.g. "DS3231", "NTP", "MILLIS")
 */
const char* rtc_getSourceName(RTCSource source);

/**
 * Save current time to EEPROM as a backup.
 * Called periodically from loop() so time survives power loss without DS3231.
 */
void rtc_saveToEEPROM();

/**
 * Get current Unix timestamp (seconds since 1970-01-01 00:00:00 UTC).
 */
unsigned long rtc_getUnixTime();

/**
 * Check if RTC has valid (non-default) time.
 * @return true if time has been set from DS3231 or NTP
 */
bool rtc_isValid();

/**
 * Get day name.
 * @param dayOfWeek - 0-6 (0=Sunday)
 * @return day name string
 */
const char* rtc_getDayName(uint8_t dayOfWeek);

/**
 * Get month name.
 * @param month - 1-12
 * @return month name string
 */
const char* rtc_getMonthName(uint8_t month);

#endif // RTC_H
