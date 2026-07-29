#include "Heater.h"

// ============================================================================
// HEATER INSTANCE - GLOBAL DATA
// ============================================================================

HeaterData heaterData;
static HeaterConfig heaterConfig;

// ============================================================================
// INITIALIZATION
// ============================================================================

void heater_init() {
  // Initialize relay pin for heater
  pinMode(HEATER_RELAY_PIN, OUTPUT);
  digitalWrite(HEATER_RELAY_PIN, LOW);

  heaterData.currentState = HeaterState::OFF;
  heaterData.isActive = false;
  heaterData.hasError = false;
  heaterData.retryCount = 0;
  heaterData.lastStateChangeMs = millis();
  heaterData.lastActivationMs = 0;
  heaterData.runTimeMs = 0;

  // Set default configuration
  heaterConfig.mode = HeaterState::OFF;
  heaterConfig.dewPointThresholdC = 15.0f;
  heaterConfig.hysteresisC = 2.0f;
  heaterConfig.debounceTimeMs = 5000;
  heaterConfig.maxRetries = 3;
  heaterConfig.retryDelayMs = 10000;
}

// ============================================================================
// HEATER CONTROL
// ============================================================================

void heater_on() {
  if (heaterData.currentState != HeaterState::ON && heaterData.currentState != HeaterState::AUTO) {
    digitalWrite(HEATER_RELAY_PIN, HIGH);
    heaterData.isActive = true;
    heaterData.lastActivationMs = millis();
  }
}

void heater_off() {
  digitalWrite(HEATER_RELAY_PIN, LOW);
  heaterData.isActive = false;
}

void heater_setMode(HeaterState mode) {
  heaterConfig.mode = mode;
  heaterData.currentState = mode;
  
  if (mode == HeaterState::OFF) {
    heater_off();
  } else if (mode == HeaterState::ON) {
    heater_on();
  }
  // AUTO mode is handled in heater_update()
}

// ============================================================================
// STATE MACHINE UPDATE
// ============================================================================

static unsigned long lastDebounceMs = 0;

void heater_update() {
  unsigned long currentTime = millis();
  unsigned long stateAge = currentTime - heaterData.lastStateChangeMs;

  // Check if cabinet temperature/humidity data is valid
  if (!cabinetEnvironment.isValid) {
    return; // Wait for valid sensor data
  }

  switch (heaterConfig.mode) {
    // ========================================================================
    case HeaterState::OFF: {
      heater_off();
      heaterData.currentState = HeaterState::OFF;
      break;
    }

    // ========================================================================
    case HeaterState::ON: {
      heater_on();
      heaterData.currentState = HeaterState::ON;
      if (heaterData.lastActivationMs > 0) {
        heaterData.runTimeMs = currentTime - heaterData.lastActivationMs;
      }
      break;
    }

    // ========================================================================
    case HeaterState::AUTO: {
      // Automatic mode based on dew point
      // Turn on when dew point rises above threshold
      // Turn off when dew point falls below (threshold - hysteresis)
      
      if (heaterData.isActive) {
        // Heater is on - check if we should turn it off
        if (cabinetEnvironment.dewPointC < (heaterConfig.dewPointThresholdC - heaterConfig.hysteresisC)) {
          // Dew point dropped below threshold - hysteresis
          if (stateAge > heaterConfig.debounceTimeMs) {
            heater_off();
            heaterData.currentState = HeaterState::AUTO;
            heaterData.lastStateChangeMs = currentTime;
          }
        }
      } else {
        // Heater is off - check if we should turn it on
        if (cabinetEnvironment.dewPointC > heaterConfig.dewPointThresholdC) {
          // Dew point rose above threshold
          if (stateAge > heaterConfig.debounceTimeMs) {
            heater_on();
            heaterData.currentState = HeaterState::AUTO;
            heaterData.lastStateChangeMs = currentTime;
          }
        }
      }

      // Update runtime
      if (heaterData.lastActivationMs > 0) {
        heaterData.runTimeMs = currentTime - heaterData.lastActivationMs;
      }
      break;
    }

    // ========================================================================
    case HeaterState::ERROR: {
      heater_off();
      heaterData.hasError = true;
      break;
    }
  }
}

// ============================================================================
// PUBLIC API
// ============================================================================

HeaterState heater_getState() {
  return heaterData.currentState;
}

bool heater_isActive() {
  return heaterData.isActive;
}

HeaterConfig* heater_getConfig() {
  return &heaterConfig;
}

const char* heater_getStateName(HeaterState state) {
  switch (state) {
    case HeaterState::OFF:   return "OFF";
    case HeaterState::ON:    return "ON";
    case HeaterState::AUTO:  return "AUTO";
    case HeaterState::ERROR: return "ERROR";
    default:                 return "UNKNOWN";
  }
}
