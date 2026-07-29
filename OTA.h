#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include "Globals.h"

// ============================================================================
// OTA STATE ENUM
// ============================================================================

enum class OTAState : uint8_t {
  IDLE = 0,           // Not updating
  CHECKING = 1,       // Checking for updates
  DOWNLOADING = 2,    // Downloading firmware
  INSTALLING = 3,     // Installing update
  COMPLETE = 4,       // Update complete
  ERROR = 5           // Update error
};

// ============================================================================
// OTA CONFIGURATION
// ============================================================================

struct OTAConfig {
  char updateServerURL[128] = "";
  uint32_t checkIntervalHours = 24;      // Check for updates every 24 hours
  bool autoUpdate = false;               // Automatically install updates
  uint32_t updateWindowStartHour = 2;    // Start update window at 2 AM
  uint32_t updateWindowEndHour = 4;      // End update window at 4 AM
};

// ============================================================================
// OTA DATA - RUNTIME STATE
// ============================================================================

struct OTAData {
  OTAState currentState = OTAState::IDLE;
  bool updateAvailable = false;
  char currentVersion[16] = "1.0.0";
  char latestVersion[16] = "1.0.0";
  uint8_t updateProgress = 0;            // 0-100%
  unsigned long lastCheckMs = 0;
  unsigned long lastUpdateMs = 0;
  bool hasError = false;
  char errorMessage[64] = "";
};

// ============================================================================
// OTA INSTANCE
// ============================================================================

extern OTAData otaData;

// ============================================================================
// OTA CONTROL FUNCTIONS
// ============================================================================

/**
 * Initialize OTA system
 */
void ota_init();

/**
 * Update OTA state machine
 * Should be called in main loop
 */
void ota_update();

/**
 * Check for available updates
 * @return - true if check initiated
 */
bool ota_checkForUpdates();

/**
 * Install available update
 * @return - true if installation initiated
 */
bool ota_installUpdate();

/**
 * Get current OTA state
 */
OTAState ota_getState();

/**
 * Check if update is available
 */
bool ota_isUpdateAvailable();

/**
 * Get update progress (0-100%)
 */
uint8_t ota_getProgress();

/**
 * Get OTA configuration
 */
OTAConfig* ota_getConfig();

/**
 * Get OTA data
 */
OTAData* ota_getData();

/**
 * Get human-readable state name
 */
const char* ota_getStateName(OTAState state);

#endif // OTA_H
