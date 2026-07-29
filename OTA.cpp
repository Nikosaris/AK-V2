#include "OTA.h"

// ============================================================================
// OTA INSTANCE - GLOBAL DATA
// ============================================================================

OTAData otaData;
static OTAConfig otaConfig;

// ============================================================================
// INITIALIZATION
// ============================================================================

void ota_init() {
  otaData.currentState = OTAState::IDLE;
  otaData.updateAvailable = false;
  strcpy(otaData.currentVersion, "1.0.0");
  strcpy(otaData.latestVersion, "1.0.0");
  otaData.updateProgress = 0;
  otaData.lastCheckMs = 0;
  otaData.lastUpdateMs = 0;
  otaData.hasError = false;
  strcpy(otaData.errorMessage, "");

  // Initialize OTA config
  strcpy(otaConfig.updateServerURL, "");
  otaConfig.checkIntervalHours = 24;
  otaConfig.autoUpdate = false;
  otaConfig.updateWindowStartHour = 2;
  otaConfig.updateWindowEndHour = 4;
}

// ============================================================================
// OTA UPDATE CHECK
// ============================================================================

bool ota_checkForUpdates() {
  otaData.currentState = OTAState::CHECKING;
  otaData.lastCheckMs = millis();

  // TODO: Implement HTTP request to update server
  // Check latest version and compare with currentVersion
  // If newer version available, set updateAvailable = true

  return true;
}

bool ota_installUpdate() {
  if (!otaData.updateAvailable) {
    return false;
  }

  otaData.currentState = OTAState::DOWNLOADING;
  otaData.lastUpdateMs = millis();
  otaData.updateProgress = 0;

  // TODO: Implement firmware download and installation
  // This requires ArduinoOTA or HTTPUpdate

  return true;
}

// ============================================================================
// STATE MACHINE UPDATE
// ============================================================================

static unsigned long lastCheckIntervalMs = 0;

void ota_update() {
  unsigned long currentTime = millis();

  switch (otaData.currentState) {
    // ========================================================================
    case OTAState::IDLE: {
      // Check if it's time to check for updates
      unsigned long checkInterval = otaConfig.checkIntervalHours * 3600000; // Convert to ms
      if ((currentTime - otaData.lastCheckMs) > checkInterval) {
        ota_checkForUpdates();
      }
      break;
    }

    // ========================================================================
    case OTAState::CHECKING: {
      // TODO: Poll for check completion
      // Once complete, return to IDLE state
      otaData.currentState = OTAState::IDLE;
      break;
    }

    // ========================================================================
    case OTAState::DOWNLOADING: {
      // TODO: Poll download progress
      // Update otaData.updateProgress
      break;
    }

    // ========================================================================
    case OTAState::INSTALLING: {
      // TODO: Monitor installation
      break;
    }

    // ========================================================================
    case OTAState::COMPLETE: {
      // Update successfully installed
      strcpy(otaData.currentVersion, otaData.latestVersion);
      otaData.currentState = OTAState::IDLE;
      break;
    }

    // ========================================================================
    case OTAState::ERROR: {
      // Wait before retrying
      unsigned long timeSinceError = currentTime - otaData.lastUpdateMs;
      if (timeSinceError > 300000) { // 5 minutes
        otaData.currentState = OTAState::IDLE;
        otaData.hasError = false;
      }
      break;
    }
  }
}

// ============================================================================
// PUBLIC API
// ============================================================================

OTAState ota_getState() {
  return otaData.currentState;
}

bool ota_isUpdateAvailable() {
  return otaData.updateAvailable;
}

uint8_t ota_getProgress() {
  return otaData.updateProgress;
}

OTAConfig* ota_getConfig() {
  return &otaConfig;
}

OTAData* ota_getData() {
  return &otaData;
}

const char* ota_getStateName(OTAState state) {
  switch (state) {
    case OTAState::IDLE:       return "IDLE";
    case OTAState::CHECKING:   return "CHECKING";
    case OTAState::DOWNLOADING: return "DOWNLOADING";
    case OTAState::INSTALLING: return "INSTALLING";
    case OTAState::COMPLETE:   return "COMPLETE";
    case OTAState::ERROR:      return "ERROR";
    default:                   return "UNKNOWN";
  }
}
