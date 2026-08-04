#ifndef MOTOR_H
#define MOTOR_H

#include "Globals.h"

// ============================================================================
// MOTOR STATE MACHINE ENUMS
// ============================================================================

enum class MotorState : uint8_t {
  STOPPED = 0,
  OPENING = 1,
  CLOSING = 2,
  OBSTACLE = 3,
  TIMEOUT = 4,
  ERROR = 5
};

enum class MotorCommand : uint8_t {
  NONE = 0,
  OPEN = 1,
  CLOSE = 2,
  STOP = 3
};

// ============================================================================
// MOTOR CONFIGURATION STRUCTURE
// ============================================================================

struct MotorConfig {
  // Timeout protection
  uint32_t timeoutMs = 30000;           // 30 seconds - max time to reach limit switch

  // Current protection
  uint16_t maxCurrent = 1500;           // mA - maximum allowed current
  uint32_t currentIgnoreTimeMs = 500;   // ms - ignore current after motor start
  uint32_t overCurrentTimeMs = 1000;    // ms - duration before triggering OBSTACLE

  // Retry logic
  uint8_t maxRetries = 3;               // number of retry attempts after obstacle

  // PWM duty cycles (0-255)
  uint8_t pwmOpen = 200;                // PWM for opening movement
  uint8_t pwmClose = 200;               // PWM for closing movement
  uint8_t pwmSlow = 100;                // PWM for slow approach to limit
  uint32_t slowApproachDistanceMs = 2000; // Time before reaching limit to slow down
};

// ============================================================================
// MOTOR DATA STRUCTURE (Runtime state)
// ============================================================================

struct MotorData {
  // State machine
  MotorState state = MotorState::STOPPED;
  MotorCommand command = MotorCommand::NONE;

  // Limit switches (reading from GPIO)
  bool topLimit = false;                // True if top limit switch is pressed
  bool bottomLimit = false;             // True if bottom limit switch is pressed

  // Current sensing
  uint16_t currentMA = 0;               // Current in milliamps
  uint32_t currentHighTimeMs = 0;       // Duration of high current detection

  // Timing and movement
  uint32_t movementStartTimeMs = 0;     // When movement started
  uint32_t elapsedTimeMs = 0;           // Time spent in current state

  // Retry counter
  uint8_t retryCount = 0;               // Current retry attempt

  // Error tracking
  bool hasError = false;                // Error flag
  const char* lastError = nullptr;      // Last error message
};

// ============================================================================
// MOTOR CONTROL STRUCTURES
// ============================================================================

struct Motor {
  MotorConfig config;
  MotorData data;
  uint8_t drivePinIn1;
  uint8_t drivePinIn2;
  const char* name;
};

extern Motor doorMotor;
extern Motor windowMotor;

// ============================================================================
// MOTOR INITIALIZATION AND UPDATE
// ============================================================================

/**
 * Initialize motor structure with default values
 * @param motor - pointer to motor structure
 * @param name - descriptive name (e.g., "Door", "Window")
 * @param drivePin1 - H-bridge drive pin for IN1
 * @param drivePin2 - H-bridge drive pin for IN2
 */
void motor_init(Motor* motor, const char* name, uint8_t drivePin1, uint8_t drivePin2);

/**
 * Update motor state machine - CORE CONTROL FUNCTION
 * - Reads limit switches and current
 * - Executes state transitions
 * - Handles timeouts, obstacles, retries
 * - Applies PWM to motor output
 * 
 * This is the ONLY place where motor state changes occur.
 * All commands go through this function.
 * 
 * @param motor - pointer to motor structure
 */
void motor_update(Motor* motor);

/**
 * Set motor command (OPEN, CLOSE, STOP)
 * Command is stored in motor.data.command and processed by motor_update()
 * 
 * @param motor - pointer to motor structure
 * @param cmd - command to execute
 */
void motor_setCommand(Motor* motor, MotorCommand cmd);

/**
 * Emergency stop - cuts power to motor immediately
 * Does NOT change state machine - only stops PWM output
 * State transitions happen in motor_update() on next call
 * 
 * @param motor - pointer to motor structure
 */
void motor_stop(Motor* motor);

/**
 * Reset motor error state
 * @param motor - pointer to motor structure
 */
void motor_resetError(Motor* motor);

/**
 * Get human-readable state name
 * @param state - motor state
 * @return - string representation of state
 */
const char* motor_getStateName(MotorState state);

/**
 * Get human-readable command name
 * @param cmd - motor command
 * @return - string representation of command
 */
const char* motor_getCommandName(MotorCommand cmd);

#endif // MOTOR_H
