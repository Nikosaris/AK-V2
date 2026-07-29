#include "Globals.h"

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

unsigned long systemUptime = 0;        // Time since startup in milliseconds
unsigned long lastLoopTime = 0;        // Timestamp of last loop execution
unsigned long lastSerialLogTime = 0;   // Timestamp of last serial log

// ============================================================================
// INITIALIZATION
// ============================================================================

void globals_init() {
  // Initialize timing variables
  lastLoopTime = millis();
  lastSerialLogTime = millis();
  systemUptime = 0;
}

// ============================================================================
// UPDATE
// ============================================================================

void globals_update() {
  unsigned long currentTime = millis();
  systemUptime = currentTime;
}
