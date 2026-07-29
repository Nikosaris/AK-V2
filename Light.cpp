#include "Light.h"

// ============================================================================
// LIGHT INSTANCE - GLOBAL DATA
// ============================================================================

LightData lightData;
static LightConfig lightConfig;

// ============================================================================
// INITIALIZATION
// ============================================================================

void light_init() {
  // Initialize relay pin for light
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  digitalWrite(LIGHT_RELAY_PIN, LOW);

  lightData.currentState = LightState::OFF;
  lightData.isActive = false;
  lightData.hasError = false;
  lightData.lastStateChangeMs = millis();
  lightData.lastActivationMs = 0;
  lightData.runTimeMs = 0;

  // Set default configuration
  lightConfig.mode = LightState::OFF;
  lightConfig.autoOnHour = 6;
  lightConfig.autoOnMinute = 0;
  lightConfig.autoOffHour = 20;
  lightConfig.autoOffMinute = 0;
  lightConfig.minOnTimeMs = 100;
}

// ============================================================================
// LIGHT CONTROL
// ============================================================================

void light_on() {
  if (lightData.currentState != LightState::ON && lightData.currentState != LightState::AUTO) {
    digitalWrite(LIGHT_RELAY_PIN, HIGH);
    lightData.isActive = true;
    lightData.lastActivationMs = millis();
  }
}

void light_off() {
  // Ensure minimum on time has elapsed
  if (lightData.lastActivationMs > 0) {
    unsigned long onTime = millis() - lightData.lastActivationMs;
    if (onTime < lightConfig.minOnTimeMs) {
      return; // Not yet ready to turn off
    }
  }

  digitalWrite(LIGHT_RELAY_PIN, LOW);
  lightData.isActive = false;
}

void light_setMode(LightState mode) {
  lightConfig.mode = mode;
  lightData.currentState = mode;

  if (mode == LightState::OFF) {
    light_off();
  } else if (mode == LightState::ON) {
    light_on();
  }
  // AUTO mode is handled in light_update()
}

// ============================================================================
// STATE MACHINE UPDATE
// ============================================================================

// TODO: This requires RTC module to get current time
// For now, auto mode will need to be implemented once RTC is available

void light_update() {
  unsigned long currentTime = millis();

  switch (lightConfig.mode) {
    // ========================================================================
    case LightState::OFF: {
      light_off();
      lightData.currentState = LightState::OFF;
      break;
    }

    // ========================================================================
    case LightState::ON: {
      light_on();
      lightData.currentState = LightState::ON;
      if (lightData.lastActivationMs > 0) {
        lightData.runTimeMs = currentTime - lightData.lastActivationMs;
      }
      break;
    }

    // ========================================================================
    case LightState::AUTO: {
      // TODO: Implement when RTC module is available
      // For now, keep in current state
      lightData.currentState = LightState::AUTO;
      break;
    }

    // ========================================================================
    case LightState::ERROR: {
      light_off();
      lightData.hasError = true;
      break;
    }
  }
}

// ============================================================================
// PUBLIC API
// ============================================================================

LightState light_getState() {
  return lightData.currentState;
}

bool light_isActive() {
  return lightData.isActive;
}

LightConfig* light_getConfig() {
  return &lightConfig;
}

const char* light_getStateName(LightState state) {
  switch (state) {
    case LightState::OFF:   return "OFF";
    case LightState::ON:    return "ON";
    case LightState::AUTO:  return "AUTO";
    case LightState::ERROR: return "ERROR";
    default:                return "UNKNOWN";
  }
}
