#include "RTC.h"
#include <Wire.h>
#include <EEPROM.h>

// ============================================================================
// PRIVATE DATA
// ============================================================================

static TimeData currentTime;
static bool     rtcValid        = false;
static bool     ds3231Available = false;
static RTCSource currentSource  = RTCSource::NONE;

// ============================================================================
// HELPER - BCD conversion
// ============================================================================

static uint8_t bcdToDec(uint8_t val) {
  return (val / 16 * 10) + (val % 16);
}

static uint8_t decToBcd(uint8_t val) {
  return (val / 10 * 16) + (val % 10);
}

// ============================================================================
// HELPER - CRC16
// ============================================================================

static uint16_t calculateCRC16(const uint8_t* data, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

// ============================================================================
// DS3231 IMPLEMENTATION
// ============================================================================

bool rtc_initDS3231() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(DS3231_I2C_CLOCK);

  // Probe DS3231 at 0x68
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  uint8_t err = Wire.endTransmission();
  if (err != 0) {
    Serial.println("[RTC] DS3231 not found on I2C 0x68");
    ds3231Available = false;
    return false;
  }

  ds3231Available = true;
  Serial.println("[RTC] \xe2\x9c\x93 DS3231 found on I2C 0x68");
  return true;
}

bool rtc_isDS3231Available() {
  return ds3231Available;
}

bool rtc_readDS3231() {
  if (!ds3231Available) {
    return false;
  }

  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x00);  // Start at register 0
  if (Wire.endTransmission() != 0) {
    return false;
  }

  Wire.requestFrom((uint8_t)DS3231_I2C_ADDRESS, (uint8_t)7);
  if (Wire.available() < 7) {
    return false;
  }

  currentTime.second    = bcdToDec(Wire.read() & 0x7F);
  currentTime.minute    = bcdToDec(Wire.read() & 0x7F);
  currentTime.hour      = bcdToDec(Wire.read() & 0x3F);
  currentTime.dayOfWeek = (Wire.read() & 0x07) - 1;  // DS3231: 1-7, we use 0-6
  currentTime.day       = bcdToDec(Wire.read() & 0x3F);
  currentTime.month     = bcdToDec(Wire.read() & 0x1F);
  currentTime.year      = bcdToDec(Wire.read());

  rtcValid = (currentTime.year >= 24);  // Sanity: year >= 2024
  return rtcValid;
}

bool rtc_writeDS3231(TimeData* time) {
  if (!ds3231Available || time == nullptr) {
    return false;
  }

  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x00);
  Wire.write(decToBcd(time->second));
  Wire.write(decToBcd(time->minute));
  Wire.write(decToBcd(time->hour));
  Wire.write(decToBcd(time->dayOfWeek + 1));  // DS3231 uses 1-7
  Wire.write(decToBcd(time->day));
  Wire.write(decToBcd(time->month));
  Wire.write(decToBcd(time->year));
  if (Wire.endTransmission() != 0) {
    return false;
  }

  Serial.println("[RTC] \xe2\x9c\x93 DS3231 updated");
  return true;
}

// ============================================================================
// EEPROM BACKUP
// ============================================================================

void rtc_saveToEEPROM() {
  uint8_t buf[7];
  buf[0] = currentTime.year;
  buf[1] = currentTime.month;
  buf[2] = currentTime.day;
  buf[3] = currentTime.hour;
  buf[4] = currentTime.minute;
  buf[5] = currentTime.second;
  buf[6] = currentTime.dayOfWeek;

  for (uint8_t i = 0; i < 7; i++) {
    EEPROM.write(EEPROM_ADDR_TIME_BACKUP + i, buf[i]);
  }

  uint16_t crc = calculateCRC16(buf, 7);
  EEPROM.write(EEPROM_ADDR_TIME_CHECKSUM,     (crc >> 8) & 0xFF);
  EEPROM.write(EEPROM_ADDR_TIME_CHECKSUM + 1, crc & 0xFF);
  EEPROM.commit();

  Serial.println("[RTC] \xe2\x9c\x93 EEPROM backup saved");
}

void rtc_loadFromEEPROM() {
  uint8_t buf[7];
  for (uint8_t i = 0; i < 7; i++) {
    buf[i] = EEPROM.read(EEPROM_ADDR_TIME_BACKUP + i);
  }

  uint16_t storedCRC = ((uint16_t)EEPROM.read(EEPROM_ADDR_TIME_CHECKSUM) << 8)
                     | EEPROM.read(EEPROM_ADDR_TIME_CHECKSUM + 1);
  uint16_t calcCRC = calculateCRC16(buf, 7);

  if (storedCRC != calcCRC) {
    Serial.println("[RTC] EEPROM CRC mismatch - backup invalid");
    return;
  }

  currentTime.year      = buf[0];
  currentTime.month     = buf[1];
  currentTime.day       = buf[2];
  currentTime.hour      = buf[3];
  currentTime.minute    = buf[4];
  currentTime.second    = buf[5];
  currentTime.dayOfWeek = buf[6];

  rtcValid      = true;
  currentSource = RTCSource::EEPROM;
  Serial.println("[RTC] \xe2\x9c\x93 Time restored from EEPROM backup");
}

bool rtc_hasValidEEPROM() {
  uint8_t buf[7];
  for (uint8_t i = 0; i < 7; i++) {
    buf[i] = EEPROM.read(EEPROM_ADDR_TIME_BACKUP + i);
  }
  uint16_t storedCRC = ((uint16_t)EEPROM.read(EEPROM_ADDR_TIME_CHECKSUM) << 8)
                     | EEPROM.read(EEPROM_ADDR_TIME_CHECKSUM + 1);
  return storedCRC == calculateCRC16(buf, 7);
}

// ============================================================================
// NTP SYNC
// ============================================================================

void rtc_syncFromNTP(TimeData* ntpTime, bool viaEthernet) {
  if (ntpTime == nullptr) return;

  // Calculate diff in seconds for logging
  int32_t diffSec = (int32_t)ntpTime->hour * 3600 + ntpTime->minute * 60 + ntpTime->second
                  - (int32_t)currentTime.hour * 3600 - currentTime.minute * 60 - currentTime.second;

  Serial.print("[RTC] NTP diff: ");
  if (diffSec >= 0) Serial.print("+");
  Serial.print(diffSec);
  Serial.println(" seconds");

  currentTime   = *ntpTime;
  rtcValid      = true;
  currentSource = viaEthernet ? RTCSource::NTP_ETHERNET : RTCSource::NTP_WIFI;

  // Write to DS3231
  rtc_writeDS3231(&currentTime);

  // Save EEPROM backup
  rtc_saveToEEPROM();

  Serial.print("[RTC] Source: ");
  Serial.println(rtc_getSourceName(currentSource));
}

void rtc_adjustTime(int32_t offsetSeconds) {
  // Simple adjustment - add seconds to current time
  int32_t totalSec = (int32_t)currentTime.second + offsetSeconds;

  // Normalize
  while (totalSec < 0)  { totalSec += 60; currentTime.minute--; }
  while (totalSec >= 60){ totalSec -= 60; currentTime.minute++; }
  currentTime.second = (uint8_t)totalSec;

  if (currentTime.minute >= 60) { currentTime.minute = 0; currentTime.hour++; }
  if (currentTime.hour   >= 24) { currentTime.hour   = 0; }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void rtc_init() {
  EEPROM.begin(512);

  Serial.println("[RTC] === Inicializace RTC ===");

  // Try DS3231
  if (rtc_initDS3231() && rtc_readDS3231()) {
    currentSource = RTCSource::DS3231;
    char buf[48];
    snprintf(buf, sizeof(buf), "[RTC] DS3231 time: 20%02d-%02d-%02d %02d:%02d:%02d",
             currentTime.year, currentTime.month, currentTime.day,
             currentTime.hour, currentTime.minute, currentTime.second);
    Serial.println(buf);
  }
  // Try EEPROM fallback
  else if (rtc_hasValidEEPROM()) {
    rtc_loadFromEEPROM();
  }
  // Last resort - millis
  else {
    currentTime = {24, 1, 1, 0, 0, 0, 1};
    currentSource = RTCSource::MILLIS;
    Serial.println("[RTC] \xe2\x9a\xa0\xef\xb8\x8f  No time source - using millis counter");
  }

  Serial.print("[RTC] Source: ");
  Serial.println(rtc_getSourceName(currentSource));
}

// ============================================================================
// UPDATE
// ============================================================================

void rtc_update() {
  static unsigned long lastSecondMs  = 0;
  static unsigned long lastDS3231Ms  = 0;
  unsigned long now = millis();

  // Increment millis-based second counter (always runs)
  if (now - lastSecondMs >= 1000) {
    lastSecondMs = now;
    if (currentSource == RTCSource::MILLIS) {
      currentTime.second++;
      if (currentTime.second >= 60) {
        currentTime.second = 0;
        currentTime.minute++;
        if (currentTime.minute >= 60) {
          currentTime.minute = 0;
          currentTime.hour++;
          if (currentTime.hour >= 24) {
            currentTime.hour = 0;
          }
        }
      }
    }
  }

  // Read DS3231 every 5 seconds
  if (ds3231Available && (now - lastDS3231Ms >= 5000)) {
    lastDS3231Ms = now;
    if (rtc_readDS3231()) {
      if (currentSource == RTCSource::DS3231 ||
          currentSource == RTCSource::EEPROM ||
          currentSource == RTCSource::MILLIS) {
        currentSource = RTCSource::DS3231;
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
  if (time == nullptr) return false;
  currentTime = *time;
  rtcValid    = true;
  rtc_writeDS3231(&currentTime);
  return true;
}

unsigned long rtc_getUnixTime() {
  // Seconds since 2000-01-01 (simplified, no leap year)
  return (unsigned long)currentTime.year * 31536000UL
       + (unsigned long)currentTime.month * 2592000UL
       + (unsigned long)currentTime.day   * 86400UL
       + (unsigned long)currentTime.hour  * 3600UL
       + (unsigned long)currentTime.minute * 60UL
       + currentTime.second;
}

bool rtc_isValid() {
  return rtcValid;
}

RTCSource rtc_getCurrentSource() {
  return currentSource;
}

const char* rtc_getSourceName(RTCSource source) {
  switch (source) {
    case RTCSource::NONE:         return "NONE";
    case RTCSource::MILLIS:       return "MILLIS";
    case RTCSource::EEPROM:       return "EEPROM";
    case RTCSource::DS3231:       return "DS3231";
    case RTCSource::NTP_WIFI:     return "NTP_WIFI";
    case RTCSource::NTP_ETHERNET: return "NTP_ETHERNET";
    default:                      return "UNKNOWN";
  }
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
