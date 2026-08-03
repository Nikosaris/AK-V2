#include "Hardware.h"

// ============================================================================
// HARDWARE INITIALIZATION
// ============================================================================

void hardware_init() {
  // Initialize digital inputs (limit switches, button)
  pinMode(DOOR_TOP_LIMIT_PIN, INPUT);
  pinMode(DOOR_BOTTOM_LIMIT_PIN, INPUT);
  pinMode(WINDOW_TOP_LIMIT_PIN, INPUT);
  pinMode(WINDOW_BOTTOM_LIMIT_PIN, INPUT);
  pinMode(LOCAL_BUTTON_PIN, INPUT);

  // Initialize motor control pins (H-Bridge)
  pinMode(DOOR_IN1_PIN, OUTPUT);
  pinMode(DOOR_IN2_PIN, OUTPUT);
  pinMode(WINDOW_IN1_PIN, OUTPUT);
  pinMode(WINDOW_IN2_PIN, OUTPUT);

  // Initialize relay outputs
  pinMode(CAMERA_RELAY_PIN, OUTPUT);
  pinMode(HEATER_RELAY_PIN, OUTPUT);
  pinMode(LIGHT_RELAY_PIN, OUTPUT);

  // Initialize analog input for current sensing
  pinMode(ACS712_PIN, INPUT);

  // Ensure all motor outputs are stopped
  digitalWrite(DOOR_IN1_PIN, LOW);
  digitalWrite(DOOR_IN2_PIN, LOW);
  digitalWrite(WINDOW_IN1_PIN, LOW);
  digitalWrite(WINDOW_IN2_PIN, LOW);

  // Ensure all relays are off
  digitalWrite(CAMERA_RELAY_PIN, LOW);
  digitalWrite(HEATER_RELAY_PIN, LOW);
  digitalWrite(LIGHT_RELAY_PIN, LOW);

  // Configure PWM for motor control using ESP32 LEDC API
  // Door motor PWM
  ledcAttach(DOOR_IN1_PIN, PWM_FREQUENCY * 1000, PWM_RESOLUTION);
  ledcAttach(DOOR_IN2_PIN, PWM_FREQUENCY * 1000, PWM_RESOLUTION);

  // Window motor PWM
  ledcAttach(WINDOW_IN1_PIN, PWM_FREQUENCY * 1000, PWM_RESOLUTION);
  ledcAttach(WINDOW_IN2_PIN, PWM_FREQUENCY * 1000, PWM_RESOLUTION);
}

// ============================================================================
// HARDWARE UPDATE
// ============================================================================

void hardware_update() {
  // This function can be used for periodic hardware maintenance
  // Currently empty, but available for future expansion
}

// ============================================================================
// HARDWARE SHUTDOWN
// ============================================================================

void hardware_shutdown() {
  // Stop all motors
  digitalWrite(DOOR_IN1_PIN, LOW);
  digitalWrite(DOOR_IN2_PIN, LOW);
  digitalWrite(WINDOW_IN1_PIN, LOW);
  digitalWrite(WINDOW_IN2_PIN, LOW);

  // Turn off all relays
  digitalWrite(CAMERA_RELAY_PIN, LOW);
  digitalWrite(HEATER_RELAY_PIN, LOW);
  digitalWrite(LIGHT_RELAY_PIN, LOW);
}
