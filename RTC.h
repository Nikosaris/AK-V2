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
// RTC SOURCE ENUM
// ============================================================================

enum class RTCSource : uint8_t {
  NONE         = 0,
  MILLIS       = 1,
  EEPROM       = 2,
  DS3231       = 3,
  NTP_WIFI     = 4,
  NTP_ETHERNET = 5
};

// ============================================================================
// RTC FUNCTIONS - Basic
// ============================================================================

void rtc_init();
void rtc_update();
TimeData* rtc_getTime();
bool rtc_setTime(TimeData* time);
unsigned long rtc_getUnixTime();
bool rtc_isValid();
const char* rtc_getDayName(uint8_t dayOfWeek);
const char* rtc_getMonthName(uint8_t month);

// ============================================================================
// RTC FUNCTIONS - DS3231 Hardware
// ============================================================================

bool rtc_initDS3231();
bool rtc_readDS3231();
bool rtc_writeDS3231(TimeData* time);
bool rtc_isDS3231Available();

// ============================================================================
// RTC FUNCTIONS - EEPROM Backup
// ============================================================================

void rtc_saveToEEPROM();
void rtc_loadFromEEPROM();
bool rtc_hasValidEEPROM();

// ============================================================================
// RTC FUNCTIONS - Source management
// ============================================================================

RTCSource rtc_getCurrentSource();
const char* rtc_getSourceName(RTCSource source);
void rtc_adjustTime(int32_t offsetSeconds);
void rtc_syncFromNTP(TimeData* ntpTime, bool viaEthernet);

#endif // RTC_H
