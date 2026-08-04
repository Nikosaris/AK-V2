#include "Logger.h"
#include <stdarg.h>

// ============================================================================
// LOG STORAGE - Circular Buffer
// ============================================================================

static LogEntry logs[MAX_LOG_ENTRIES];
static uint16_t logIndex = 0;       // Current write position
static uint16_t logCount = 0;       // Total logs written
static LogLevel minLogLevel = LogLevel::DEBUG;  // Minimum level to log

// ============================================================================
// INITIALIZATION
// ============================================================================

void logger_init() {
  logIndex = 0;
  logCount = 0;
  minLogLevel = LogLevel::DEBUG;

  // Clear log buffer
  for (uint16_t i = 0; i < MAX_LOG_ENTRIES; i++) {
    logs[i].level = LogLevel::DEBUG;
    logs[i].timeStampMs = 0;
    logs[i].module = nullptr;
    logs[i].message = nullptr;
  }
}

// ============================================================================
// LOGGING FUNCTIONS
// ============================================================================

void logger_log(LogLevel level, const char* module, const char* message) {
  // Check if we should log this level
  if (level < minLogLevel) {
    return;
  }

  // Write to circular buffer
  logs[logIndex].level = level;
  logs[logIndex].timeStampMs = millis();
  logs[logIndex].module = module;
  logs[logIndex].message = message;

  // Print to Serial immediately
  Serial.print("[");
  Serial.print(logger_getLevelName(level));
  Serial.print("] ");
  Serial.print(module);
  Serial.print(": ");
  Serial.println(message);

  // Advance circular buffer
  logIndex = (logIndex + 1) % MAX_LOG_ENTRIES;
  logCount++;
}

void logger_debug(const char* module, const char* message) {
  logger_log(LogLevel::DEBUG, module, message);
}

void logger_info(const char* module, const char* message) {
  logger_log(LogLevel::INFO, module, message);
}

void logger_warning(const char* module, const char* message) {
  logger_log(LogLevel::WARNING, module, message);
}

void logger_error(const char* module, const char* message) {
  logger_log(LogLevel::ERROR, module, message);
}

void logger_critical(const char* module, const char* message) {
  logger_log(LogLevel::CRITICAL, module, message);
}

// ============================================================================
// LOG QUERIES
// ============================================================================

LogEntry* logger_getEntry(uint16_t index) {
  if (index >= MAX_LOG_ENTRIES) {
    return nullptr;
  }
  return &logs[index];
}

uint16_t logger_getCount() {
  return logCount;
}

void logger_clearAll() {
  logIndex = 0;
  logCount = 0;
  for (uint16_t i = 0; i < MAX_LOG_ENTRIES; i++) {
    logs[i].level = LogLevel::DEBUG;
    logs[i].timeStampMs = 0;
    logs[i].module = nullptr;
    logs[i].message = nullptr;
  }
}

void logger_printAll() {
  Serial.println("
========== LOG BUFFER ==========");
  for (uint16_t i = 0; i < MAX_LOG_ENTRIES; i++) {
    if (logs[i].message != nullptr) {
      Serial.print(logs[i].timeStampMs);
      Serial.print(" [" );
      Serial.print(logger_getLevelName(logs[i].level));
      Serial.print("] ");
      Serial.print(logs[i].module);
      Serial.print(": ");
      Serial.println(logs[i].message);
    }
  }
  Serial.println("===============================");
}

const char* logger_getLevelName(LogLevel level) {
  switch (level) {
    case LogLevel::DEBUG:    return "DEBUG";
    case LogLevel::INFO:     return "INFO";
    case LogLevel::WARNING:  return "WARN";
    case LogLevel::ERROR:    return "ERROR";
    case LogLevel::CRITICAL: return "CRIT";
    default:                 return "???";
  }
}
