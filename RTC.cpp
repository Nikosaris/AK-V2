#include "RTC.h"
#include <Wire.h>
#include <EEPROM.h>

// ============================================================================
// RTC DATA AND CONSTANTS
// ============================================================================

static TimeData currentTime;
static bool rtcValid = false;
static RTCSource currentSource = RTCSource::MILLIS;
static unsigned long lastMillisUpdateMs = 0;

const uint8_t DS3231_ADDRESS = 0x68;

// ============================================================================
// INTERNAL HELPERS — CALENDAR
// ============================================================================

static bool isLeapYear(uint8_t year2digit) {
  uint16_t year = 2000u + year2digit;
  return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
}

static uint8_t daysInMonth(uint8_t month, uint8_t year2digit) {
  switch (month) {
    case 1:  return 31;
    case 2:  return isLeapYear(year2digit) ? 29 : 28;
    case 3:  return 31;
    case 4:  return 30;
    case 5:  return 31;
    case 6:  return 30;
    case 7:  return 31;
    case 8:  return 31;
    case 9:  return 30;
    case 10: return 31;
    case 11: return 30;
    case 12: return 31;
    default: return 30;
  }
}

// Advance currentTime by one second with full calendar rollover.
static void advanceOneSecond() {
  if (++currentTime.second < 60) return;
  currentTime.second = 0;

  if (++currentTime.minute < 60) return;
  currentTime.minute = 0;

  if (++currentTime.hour < 24) return;
  currentTime.hour = 0;

  // Advance day — keep month/day valid even if previously at defaults.
  if (currentTime.month < 1 || currentTime.month > 12) currentTime.month = 1;
  if (currentTime.day  < 1)                            currentTime.day   = 1;

  if (++currentTime.day <= daysInMonth(currentTime.month, currentTime.year)) return;
  currentTime.day = 1;

  if (++currentTime.month <= 12) return;
  currentTime.month = 1;

  if (++currentTime.year >= 100) currentTime.year = 0;
}

// ============================================================================
// INTERNAL HELPERS — DS3231 I2C
// ============================================================================

static uint8_t bcdToDec(uint8_t bcd) {
  return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint8_t decToBcd(uint8_t dec) {
  return ((dec / 10) << 4) | (dec % 10);
}

// Returns true if DS3231 responded on I2C bus.
static bool ds3231_probe() {
  Wire.beginTransmission(DS3231_ADDRESS);
  return (Wire.endTransmission() == 0);
}

// Read time registers from DS3231 into currentTime.
// Returns true on success.
static bool ds3231_readTime() {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write(0x00); // start at register 0
  if (Wire.endTransmission() != 0) return false;

  if (Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)7) != 7) return false;

  currentTime.second    = bcdToDec(Wire.read() & 0x7F);
  currentTime.minute    = bcdToDec(Wire.read() & 0x7F);
  currentTime.hour      = bcdToDec(Wire.read() & 0x3F); // 24h mode
  currentTime.dayOfWeek = bcdToDec(Wire.read() & 0x07) - 1; // DS3231 1-7 → 0-6
  currentTime.day       = bcdToDec(Wire.read() & 0x3F);
  currentTime.month     = bcdToDec(Wire.read() & 0x1F);
  currentTime.year      = bcdToDec(Wire.read());
  return true;
}

// Write currentTime to DS3231.
// Returns true on success.
static bool ds3231_writeTime() {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write(0x00);
  Wire.write(decToBcd(currentTime.second));
  Wire.write(decToBcd(currentTime.minute));
  Wire.write(decToBcd(currentTime.hour));
  Wire.write(decToBcd((currentTime.dayOfWeek & 0x07) + 1)); // 0-6 → 1-7
  Wire.write(decToBcd(currentTime.day));
  Wire.write(decToBcd(currentTime.month));
  Wire.write(decToBcd(currentTime.year));
  return (Wire.endTransmission() == 0);
}

// ============================================================================
// INTERNAL HELPERS — NTP / UNIX TIME CONVERSION
// ============================================================================

// Days from 1970-01-01 to 2000-01-01.
static const unsigned long UNIX_EPOCH_2000 = 946684800UL;

// Days per month in a non-leap year (1-indexed, index 0 unused).
static const uint8_t kDaysInMonth[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

static void unixToTimeData(unsigned long unixSec, TimeData* out) {
  // Days since 1970-01-01
  unsigned long days = unixSec / 86400UL;
  unsigned long rem  = unixSec % 86400UL;

  out->hour   = (uint8_t)(rem / 3600);
  rem        %= 3600;
  out->minute = (uint8_t)(rem / 60);
  out->second = (uint8_t)(rem % 60);

  // day of week (1970-01-01 was Thursday = 4)
  out->dayOfWeek = (uint8_t)((days + 4) % 7);

  // Convert days to year/month/day
  uint16_t year = 1970;
  while (true) {
    bool leap = (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
    uint16_t diy = leap ? 366 : 365;
    if (days < diy) break;
    days -= diy;
    year++;
  }
  out->year = (uint8_t)(year >= 2000 ? year - 2000 : 0);

  bool leap = (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
  uint8_t month = 1;
  while (month <= 12) {
    uint8_t dim = kDaysInMonth[month];
    if (month == 2 && leap) dim = 29;
    if (days < dim) break;
    days -= dim;
    month++;
  }
  out->month = month;
  out->day   = (uint8_t)(days + 1);
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void rtc_init() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Default safe time (2024-01-01 00:00:00 Monday)
  currentTime.year      = 24;
  currentTime.month     = 1;
  currentTime.day       = 1;
  currentTime.hour      = 0;
  currentTime.minute    = 0;
  currentTime.second    = 0;
  currentTime.dayOfWeek = 1;
  rtcValid = false;
  currentSource = RTCSource::MILLIS;
  lastMillisUpdateMs = millis();

  // Try to read time from DS3231
  if (ds3231_probe()) {
    if (ds3231_readTime()) {
      rtcValid = true;
      currentSource = RTCSource::DS3231;
      Serial.println("[RTC] DS3231 found and time loaded");
    } else {
      Serial.println("[RTC] DS3231 found but read failed — using MILLIS fallback");
    }
  } else {
    Serial.println("[RTC] DS3231 not found — using MILLIS fallback");
  }

  lastMillisUpdateMs = millis();
}

// ============================================================================
// TIME MANAGEMENT
// ============================================================================

void rtc_update() {
  if (currentSource == RTCSource::DS3231) {
    // Re-read DS3231 every second to stay in sync
    static unsigned long lastDS3231ReadMs = 0;
    unsigned long now = millis();
    if (now - lastDS3231ReadMs >= 1000) {
      lastDS3231ReadMs = now;
      ds3231_readTime(); // best-effort; ignore failure
    }
  } else {
    // MILLIS or NTP source: advance software clock one tick per second
    unsigned long now = millis();
    while (now - lastMillisUpdateMs >= 1000) {
      lastMillisUpdateMs += 1000;
      advanceOneSecond();
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
  if (time == nullptr) return false;

  currentTime = *time;
  rtcValid    = true;
  lastMillisUpdateMs = millis();

  // Write to hardware if present
  if (currentSource == RTCSource::DS3231 || ds3231_probe()) {
    ds3231_writeTime();
    currentSource = RTCSource::DS3231;
  }
  return true;
}

bool rtc_syncFromNTP(unsigned long unixTime, int32_t timezoneOffsetSeconds) {
  if (unixTime < UNIX_EPOCH_2000) return false; // Sanity: must be after year 2000

  // Apply timezone offset in signed arithmetic to avoid unsigned wrap-around.
  long signedUnix = (long)unixTime + (long)timezoneOffsetSeconds;
  if (signedUnix < (long)UNIX_EPOCH_2000) return false; // Guard against underflow

  unsigned long localUnix = (unsigned long)signedUnix;
  TimeData t;
  unixToTimeData(localUnix, &t);

  currentTime = t;
  rtcValid    = true;
  currentSource = RTCSource::NTP;
  lastMillisUpdateMs = millis();

  // Write back to DS3231 to persist time across reboots
  if (ds3231_probe()) {
    ds3231_writeTime();
    currentSource = RTCSource::DS3231;
    Serial.println("[RTC] NTP time written to DS3231");
  } else {
    Serial.println("[RTC] NTP time set (no DS3231 to persist)");
  }
  return true;
}

bool rtc_syncFromNTP(TimeData* time) {
  if (time == nullptr) return false;

  currentTime = *time;
  rtcValid    = true;
  currentSource = RTCSource::NTP;
  lastMillisUpdateMs = millis();

  // Write back to DS3231 to persist time across reboots
  if (ds3231_probe()) {
    ds3231_writeTime();
    currentSource = RTCSource::DS3231;
    Serial.println("[RTC] NTP time (TimeData) written to DS3231");
  } else {
    Serial.println("[RTC] NTP time (TimeData) set (no DS3231 to persist)");
  }
  return true;
}

RTCSource rtc_getSource() {
  return currentSource;
}

RTCSource rtc_getCurrentSource() {
  return currentSource;
}

const char* rtc_getSourceName(RTCSource source) {
  switch (source) {
    case RTCSource::DS3231: return "DS3231";
    case RTCSource::NTP:    return "NTP";
    case RTCSource::MILLIS: return "MILLIS";
    default:                return "UNKNOWN";
  }
}

void rtc_saveToEEPROM() {
  // EEPROM backup — writes current time so it survives power loss.
  // Uses EEPROM addresses 0-7 (1 byte each field).
  // Format: [magic(0xAB)] [year] [month] [day] [hour] [minute] [second] [dayOfWeek]
  const uint8_t EEPROM_MAGIC   = 0xAB;
  const int     EEPROM_ADDRESS = 0;

  EEPROM.write(EEPROM_ADDRESS + 0, EEPROM_MAGIC);
  EEPROM.write(EEPROM_ADDRESS + 1, currentTime.year);
  EEPROM.write(EEPROM_ADDRESS + 2, currentTime.month);
  EEPROM.write(EEPROM_ADDRESS + 3, currentTime.day);
  EEPROM.write(EEPROM_ADDRESS + 4, currentTime.hour);
  EEPROM.write(EEPROM_ADDRESS + 5, currentTime.minute);
  EEPROM.write(EEPROM_ADDRESS + 6, currentTime.second);
  EEPROM.write(EEPROM_ADDRESS + 7, currentTime.dayOfWeek);
  EEPROM.commit();
}

unsigned long rtc_getUnixTime() {
  // Approximate Unix time from currentTime (UTC representation may drift
  // by timezone offset if NTP was used with offset, but good enough for
  // day-rollover and logging purposes).
  unsigned long days = 0;
  for (uint16_t y = 0; y < (uint16_t)currentTime.year; y++) {
    days += isLeapYear((uint8_t)y) ? 366 : 365;
  }
  uint8_t m;
  bool leap = isLeapYear(currentTime.year);
  for (m = 1; m < currentTime.month; m++) {
    days += kDaysInMonth[m];
    if (m == 2 && leap) days++;
  }
  days += currentTime.day - 1;
  return UNIX_EPOCH_2000 + days * 86400UL
       + (unsigned long)currentTime.hour   * 3600UL
       + (unsigned long)currentTime.minute * 60UL
       + currentTime.second;
}

bool rtc_isValid() {
  return rtcValid;
}

const char* rtc_getDayName(uint8_t dayOfWeek) {
  const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                        "Thursday", "Friday", "Saturday"};
  if (dayOfWeek > 6) return "Unknown";
  return days[dayOfWeek];
}

const char* rtc_getMonthName(uint8_t month) {
  const char* months[] = {"Invalid", "January", "February", "March",
                          "April", "May", "June", "July", "August",
                          "September", "October", "November", "December"};
  if (month > 12) return "Invalid";
  return months[month];
}
