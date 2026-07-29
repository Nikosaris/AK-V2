#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "Globals.h"

// ============================================================================
// LOG LEVEL ENUM
// ============================================================================

enum class LogLevel : uint8_t {
  DEBUG = 0,
  INFO = 1,
  WARNING = 2,
  ERROR = 3,
  CRITICAL = 4
};

// ============================================================================
// LOG ENTRY STRUCTURE
// ============================================================================

struct LogEntry {
  LogLevel level;
  unsigned long timeStampMs;
  const char* module;        // Module name (e.g., "Motor", "Sensors")
  const char* message;       // Log message
};

// ============================================================================
// LOGGER CONFIGURATION
// ============================================================================

const uint16_t MAX_LOG_ENTRIES = 1000;  // Circular buffer size

// ============================================================================
// LOGGER FUNCTIONS
// ============================================================================

/**
 * Initialize logger system
 */
void logger_init();

/**
 * Log a message
 * @param level - log level
 * @param module - module name
 * @param message - log message
 */
void logger_log(LogLevel level, const char* module, const char* message);

/**
 * Log debug message
 */
void logger_debug(const char* module, const char* message);

/**
 * Log info message
 */
void logger_info(const char* module, const char* message);

/**
 * Log warning message
 */
void logger_warning(const char* module, const char* message);

/**
 * Log error message
 */
void logger_error(const char* module, const char* message);

/**
 * Log critical message
 */
void logger_critical(const char* module, const char* message);

/**
 * Get log entry
 * @param index - log index
 * @return - pointer to LogEntry
 */
LogEntry* logger_getEntry(uint16_t index);

/**
 * Get total log count
 */
uint16_t logger_getCount();

/**
 * Clear all logs
 */
void logger_clearAll();

/**
 * Print all logs to serial
 */
void logger_printAll();

/**
 * Get human-readable log level name
 */
const char* logger_getLevelName(LogLevel level);

#endif // LOGGER_H
