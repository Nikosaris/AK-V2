#ifndef ALARM_H
#define ALARM_H

#include <Arduino.h>
#include "Globals.h"

// ============================================================================
// ALARM TYPES
// ============================================================================

enum class AlarmType : uint8_t {
  INFO = 0,          // Information message
  WARNING = 1,       // Warning
  ERROR = 2,         // Error
  CRITICAL = 3       // Critical error
};

// ============================================================================
// ALARM STATE ENUM
// ============================================================================

enum class AlarmState : uint8_t {
  INACTIVE = 0,      // No alarm
  ACTIVE = 1,        // Alarm is active
  ACKNOWLEDGED = 2   // Alarm acknowledged by user
};

// ============================================================================
// ALARM STRUCTURE
// ============================================================================

struct AlarmEntry {
  uint8_t id;                        // Unique alarm ID
  AlarmType type;                    // Alarm type
  AlarmState state;                  // Current state
  const char* message;               // Alarm message
  unsigned long timeStampMs;         // When alarm occurred
  bool acknowledged;                 // Has user acknowledged?
};

// ============================================================================
// ALARM CONFIGURATION
// ============================================================================

const uint8_t MAX_ALARMS = 16;      // Maximum number of stored alarms

// ============================================================================
// ALARM FUNCTIONS
// ============================================================================

/**
 * Initialize alarm system
 */
void alarm_init();

/**
 * Update alarm system
 * Should be called in main loop
 */
void alarm_update();

/**
 * Trigger an alarm
 * @param type - alarm type (INFO, WARNING, ERROR, CRITICAL)
 * @param message - alarm message
 * @return - alarm ID
 */
uint8_t alarm_trigger(AlarmType type, const char* message);

/**
 * Acknowledge an alarm
 * @param alarmId - ID of alarm to acknowledge
 */
void alarm_acknowledge(uint8_t alarmId);

/**
 * Clear all alarms
 */
void alarm_clearAll();

/**
 * Get alarm entry
 * @param alarmId - alarm ID
 * @return - pointer to AlarmEntry, or nullptr if not found
 */
AlarmEntry* alarm_getEntry(uint8_t alarmId);

/**
 * Get total number of active alarms
 * @return - count of active alarms
 */
uint8_t alarm_getActiveCount();

/**
 * Check if there are critical alarms
 * @return - true if any critical alarms exist
 */
bool alarm_hasCritical();

/**
 * Get human-readable alarm type name
 */
const char* alarm_getTypeName(AlarmType type);

#endif // ALARM_H
