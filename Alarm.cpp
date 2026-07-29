#include "Alarm.h"

// ============================================================================
// ALARM STORAGE
// ============================================================================

static AlarmEntry alarms[MAX_ALARMS];
static uint8_t alarmCount = 0;
static uint8_t nextAlarmId = 1;

// ============================================================================
// INITIALIZATION
// ============================================================================

void alarm_init() {
  alarmCount = 0;
  nextAlarmId = 1;

  // Clear all alarm entries
  for (uint8_t i = 0; i < MAX_ALARMS; i++) {
    alarms[i].id = 0;
    alarms[i].type = AlarmType::INFO;
    alarms[i].state = AlarmState::INACTIVE;
    alarms[i].message = nullptr;
    alarms[i].timeStampMs = 0;
    alarms[i].acknowledged = false;
  }
}

// ============================================================================
// ALARM MANAGEMENT
// ============================================================================

uint8_t alarm_trigger(AlarmType type, const char* message) {
  // Find empty slot
  if (alarmCount >= MAX_ALARMS) {
    // Alarm buffer full - overwrite oldest non-critical alarm
    for (uint8_t i = 0; i < MAX_ALARMS; i++) {
      if (alarms[i].type != AlarmType::CRITICAL) {
        alarmCount--;
        break;
      }
    }
  }

  // Find next empty slot
  for (uint8_t i = 0; i < MAX_ALARMS; i++) {
    if (alarms[i].state == AlarmState::INACTIVE) {
      alarms[i].id = nextAlarmId;
      alarms[i].type = type;
      alarms[i].state = AlarmState::ACTIVE;
      alarms[i].message = message;
      alarms[i].timeStampMs = millis();
      alarms[i].acknowledged = false;
      alarmCount++;
      nextAlarmId++;
      return alarms[i].id;
    }
  }

  return 0; // Error - no space
}

void alarm_acknowledge(uint8_t alarmId) {
  for (uint8_t i = 0; i < MAX_ALARMS; i++) {
    if (alarms[i].id == alarmId) {
      alarms[i].state = AlarmState::ACKNOWLEDGED;
      alarms[i].acknowledged = true;
      return;
    }
  }
}

void alarm_clearAll() {
  for (uint8_t i = 0; i < MAX_ALARMS; i++) {
    alarms[i].state = AlarmState::INACTIVE;
    alarms[i].id = 0;
    alarms[i].message = nullptr;
  }
  alarmCount = 0;
}

// ============================================================================
// ALARM QUERIES
// ============================================================================

void alarm_update() {
  // Future: Implement alarm timeout or auto-clearing logic
}

AlarmEntry* alarm_getEntry(uint8_t alarmId) {
  for (uint8_t i = 0; i < MAX_ALARMS; i++) {
    if (alarms[i].id == alarmId) {
      return &alarms[i];
    }
  }
  return nullptr;
}

uint8_t alarm_getActiveCount() {
  uint8_t count = 0;
  for (uint8_t i = 0; i < MAX_ALARMS; i++) {
    if (alarms[i].state != AlarmState::INACTIVE) {
      count++;
    }
  }
  return count;
}

bool alarm_hasCritical() {
  for (uint8_t i = 0; i < MAX_ALARMS; i++) {
    if (alarms[i].type == AlarmType::CRITICAL && alarms[i].state != AlarmState::INACTIVE) {
      return true;
    }
  }
  return false;
}

const char* alarm_getTypeName(AlarmType type) {
  switch (type) {
    case AlarmType::INFO:     return "INFO";
    case AlarmType::WARNING:  return "WARNING";
    case AlarmType::ERROR:    return "ERROR";
    case AlarmType::CRITICAL: return "CRITICAL";
    default:                  return "UNKNOWN";
  }
}
