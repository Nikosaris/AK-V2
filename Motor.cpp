#include "Motor.h"

// ============================================================================
// MOTOR INITIALIZATION
// ============================================================================

void motor_init(Motor* motor, const char* name, uint8_t pwmCh1, uint8_t pwmCh2) {
  motor->name = name;
  motor->pwmChannelIn1 = pwmCh1;
  motor->pwmChannelIn2 = pwmCh2;
  motor->data.state = MotorState::STOPPED;
  motor->data.command = MotorCommand::NONE;
  motor->data.topLimit = false;
  motor->data.bottomLimit = false;
  motor->data.currentMA = 0;
  motor->data.retryCount = 0;
  motor->data.hasError = false;
  motor->data.lastError = nullptr;
}

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

/**
 * Stop motor output (cut power to H-Bridge)
 * Does NOT change state - only stops PWM
 */
static void motor_stopOutput(Motor* motor) {
  ledcWrite(motor->pwmChannelIn1, 0);
  ledcWrite(motor->pwmChannelIn2, 0);
}

/**
 * Set motor to open direction with given PWM
 */
static void motor_driveOpen(Motor* motor, uint8_t pwm) {
  ledcWrite(motor->pwmChannelIn1, pwm);
  ledcWrite(motor->pwmChannelIn2, 0);
}

/**
 * Set motor to close direction with given PWM
 */
static void motor_driveClose(Motor* motor, uint8_t pwm) {
  ledcWrite(motor->pwmChannelIn1, 0);
  ledcWrite(motor->pwmChannelIn2, pwm);
}

/**
 * Read analog current from ACS712 and convert to mA
 * Returns filtered current value
 */
static uint16_t motor_readCurrent() {
  // Simple reading - can be improved with filtering
  uint16_t rawValue = analogRead(ACS712_PIN);
  // Convert ADC value to mA (simplified conversion)
  // ACS712-5A: 185 mV/A, 2.5V offset
  // ADC: 12-bit (0-4095) for 0-3.3V
  uint16_t currentMA = ((rawValue * 3300 / 4095) - 2500) / 185;
  return currentMA > 5000 ? 0 : currentMA;  // Avoid invalid readings
}

// ============================================================================
// PUBLIC API
// ============================================================================

void motor_setCommand(Motor* motor, MotorCommand cmd) {
  motor->data.command = cmd;
}

void motor_stop(Motor* motor) {
  motor_stopOutput(motor);
}

void motor_resetError(Motor* motor) {
  if (motor->data.state == MotorState::ERROR) {
    motor->data.state = MotorState::STOPPED;
    motor->data.hasError = false;
    motor->data.lastError = nullptr;
    motor->data.retryCount = 0;
    motor_stopOutput(motor);
  }
}

// ============================================================================
// MAIN STATE MACHINE - motor_update()
// ============================================================================

void motor_update(Motor* motor) {
  unsigned long currentTime = millis();

  // Calculate elapsed time in current state
  if (motor->data.movementStartTimeMs > 0) {
    motor->data.elapsedTimeMs = currentTime - motor->data.movementStartTimeMs;
  }

  // Read limit switches
  if (motor->name[0] == 'D') {  // Door (active LOW)
    motor->data.topLimit = (digitalRead(DOOR_TOP_LIMIT_PIN) == LOW);
    motor->data.bottomLimit = (digitalRead(DOOR_BOTTOM_LIMIT_PIN) == LOW);
  } else {  // Window (active HIGH)
    motor->data.topLimit = (digitalRead(WINDOW_TOP_LIMIT_PIN) == HIGH);
    motor->data.bottomLimit = (digitalRead(WINDOW_BOTTOM_LIMIT_PIN) == HIGH);
  }

  // Read current
  motor->data.currentMA = motor_readCurrent();

  // Track overcurrent duration
  if (motor->data.state == MotorState::OPENING || motor->data.state == MotorState::CLOSING) {
    if (motor->data.elapsedTimeMs > motor->config.currentIgnoreTimeMs) {
      if (motor->data.currentMA > motor->config.maxCurrent) {
        motor->data.currentHighTimeMs += LOOP_INTERVAL_MS;
      } else {
        motor->data.currentHighTimeMs = 0;
      }
    }
  }

  // STATE MACHINE LOGIC
  switch (motor->data.state) {
    // ========================================================================
    case MotorState::STOPPED: {
      motor_stopOutput(motor);

      if (motor->data.command == MotorCommand::OPEN) {
        motor->data.state = MotorState::OPENING;
        motor->data.movementStartTimeMs = currentTime;
        motor->data.currentHighTimeMs = 0;
        motor->data.elapsedTimeMs = 0;
        motor_driveOpen(motor, motor->config.pwmOpen);

      } else if (motor->data.command == MotorCommand::CLOSE) {
        motor->data.state = MotorState::CLOSING;
        motor->data.movementStartTimeMs = currentTime;
        motor->data.currentHighTimeMs = 0;
        motor->data.elapsedTimeMs = 0;
        motor_driveClose(motor, motor->config.pwmClose);
      }
      break;
    }

    // ========================================================================
    case MotorState::OPENING: {
      // Check if reached top limit
      if (motor->data.topLimit) {
        motor_stopOutput(motor);
        motor->data.state = MotorState::STOPPED;
        motor->data.command = MotorCommand::NONE;
        motor->data.retryCount = 0;
        break;
      }

      // Check timeout
      if (motor->data.elapsedTimeMs > motor->config.timeoutMs) {
        motor_stopOutput(motor);
        motor->data.state = MotorState::TIMEOUT;
        motor->data.hasError = true;
        motor->data.lastError = "Timeout";
        break;
      }

      // Check overcurrent (obstacle detection)
      if (motor->data.currentHighTimeMs > motor->config.overCurrentTimeMs) {
        motor_stopOutput(motor);
        motor->data.state = MotorState::OBSTACLE;
        motor->data.currentHighTimeMs = 0;
        break;
      }

      // Stop command
      if (motor->data.command == MotorCommand::STOP) {
        motor_stopOutput(motor);
        motor->data.state = MotorState::STOPPED;
        motor->data.command = MotorCommand::NONE;
        break;
      }
      break;
    }

    // ========================================================================
    case MotorState::CLOSING: {
      // Check if reached bottom limit
      if (motor->data.bottomLimit) {
        motor_stopOutput(motor);
        motor->data.state = MotorState::STOPPED;
        motor->data.command = MotorCommand::NONE;
        motor->data.retryCount = 0;
        break;
      }

      // Check timeout
      if (motor->data.elapsedTimeMs > motor->config.timeoutMs) {
        motor_stopOutput(motor);
        motor->data.state = MotorState::TIMEOUT;
        motor->data.hasError = true;
        motor->data.lastError = "Timeout";
        break;
      }

      // Check overcurrent (obstacle detection)
      if (motor->data.currentHighTimeMs > motor->config.overCurrentTimeMs) {
        motor_stopOutput(motor);
        motor->data.state = MotorState::OBSTACLE;
        motor->data.currentHighTimeMs = 0;
        break;
      }

      // Stop command
      if (motor->data.command == MotorCommand::STOP) {
        motor_stopOutput(motor);
        motor->data.state = MotorState::STOPPED;
        motor->data.command = MotorCommand::NONE;
        break;
      }
      break;
    }

    // ========================================================================
    case MotorState::OBSTACLE: {
      // Retry logic
      if (motor->data.retryCount < motor->config.maxRetries) {
        motor->data.retryCount++;
        
        // Re-issue the original command
        if (motor->data.command == MotorCommand::OPEN) {
          motor->data.state = MotorState::OPENING;
          motor->data.movementStartTimeMs = currentTime;
          motor->data.currentHighTimeMs = 0;
          motor_driveOpen(motor, motor->config.pwmOpen);
        } else if (motor->data.command == MotorCommand::CLOSE) {
          motor->data.state = MotorState::CLOSING;
          motor->data.movementStartTimeMs = currentTime;
          motor->data.currentHighTimeMs = 0;
          motor_driveClose(motor, motor->config.pwmClose);
        }
      } else {
        // Max retries exceeded
        motor_stopOutput(motor);
        motor->data.state = MotorState::ERROR;
        motor->data.hasError = true;
        motor->data.lastError = "Max retries exceeded";
      }
      break;
    }

    // ========================================================================
    case MotorState::TIMEOUT: {
      motor_stopOutput(motor);
      // Transition to ERROR after timeout is detected
      motor->data.state = MotorState::ERROR;
      break;
    }

    // ========================================================================
    case MotorState::ERROR: {
      motor_stopOutput(motor);
      motor->data.command = MotorCommand::NONE;
      // Stay in ERROR until reset by motor_resetError()
      break;
    }
  }
}

// ============================================================================
// STRING CONVERSION FUNCTIONS
// ============================================================================

const char* motor_getStateName(MotorState state) {
  switch (state) {
    case MotorState::STOPPED:  return "STOPPED";
    case MotorState::OPENING:  return "OPENING";
    case MotorState::CLOSING:  return "CLOSING";
    case MotorState::OBSTACLE: return "OBSTACLE";
    case MotorState::TIMEOUT:  return "TIMEOUT";
    case MotorState::ERROR:    return "ERROR";
    default:                   return "UNKNOWN";
  }
}

const char* motor_getCommandName(MotorCommand cmd) {
  switch (cmd) {
    case MotorCommand::NONE:  return "NONE";
    case MotorCommand::OPEN:  return "OPEN";
    case MotorCommand::CLOSE: return "CLOSE";
    case MotorCommand::STOP:  return "STOP";
    default:                  return "UNKNOWN";
  }
}
