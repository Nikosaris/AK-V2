#include "Door.h"

#include "Globals.h"
#include "Sensors.h"

// ---------------------------------------------------------------------------
// PIN ASSIGNMENT
// ---------------------------------------------------------------------------

constexpr uint8_t DOOR_TOP_LIMIT = DOOR_TOP_LIMIT_PIN;
constexpr uint8_t DOOR_BOTTOM_LIMIT = DOOR_BOTTOM_LIMIT_PIN;

// ---------------------------------------------------------------------------
// INTERNAL DATA
// ---------------------------------------------------------------------------

static DoorState doorState = DoorState::UNKNOWN;
static DoorConfig doorConfig;
static DoorData doorData;

static unsigned long stateStartTime = 0;
static unsigned long movementStartTime = 0;
static bool doorError = false;
static uint8_t retryCount = 0;

// ---------------------------------------------------------------------------
// HELPERS
// ---------------------------------------------------------------------------

static void door_syncData() {
  doorData.state = doorState;
  doorData.isOpen = (doorState == DoorState::OPEN);
  doorData.isClosed = (doorState == DoorState::CLOSED);
  doorData.hasError = doorError || doorState == DoorState::OBSTACLE || doorState == DoorState::TIMEOUT || doorState == DoorState::ERROR;
  doorData.isMoving = door_isMoving();
  doorData.currentMA = sensors_getMotorCurrentMA();
  doorData.retryCount = retryCount;
  doorData.stateStartMs = stateStartTime;
  doorData.movementStartMs = movementStartTime;
}

static void door_stopOutput() {
  ledcWrite(DOOR_IN1_PIN, 0);
  ledcWrite(DOOR_IN2_PIN, 0);
}

static void door_driveOpen(uint8_t power) {
  ledcWrite(DOOR_IN1_PIN, power);
  ledcWrite(DOOR_IN2_PIN, 0);
}

static void door_driveClose(uint8_t power) {
  ledcWrite(DOOR_IN1_PIN, 0);
  ledcWrite(DOOR_IN2_PIN, power);
}

// ---------------------------------------------------------------------------
// LIMIT SWITCHES
// ---------------------------------------------------------------------------

bool door_isTopLimitActive() {
  return digitalRead(DOOR_TOP_LIMIT);
}

bool door_isBottomLimitActive() {
  return digitalRead(DOOR_BOTTOM_LIMIT);
}

// ---------------------------------------------------------------------------
// INITIALIZATION
// ---------------------------------------------------------------------------

void door_init() {
  pinMode(DOOR_TOP_LIMIT, INPUT);
  pinMode(DOOR_BOTTOM_LIMIT, INPUT);

  ledcAttach(DOOR_IN1_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttach(DOOR_IN2_PIN, PWM_FREQUENCY, PWM_RESOLUTION);

  door_stopOutput();

  if (door_isTopLimitActive()) {
    doorState = DoorState::OPEN;
    Serial.println("DOOR STARTUP -> OPEN");
  } else if (door_isBottomLimitActive()) {
    doorState = DoorState::CLOSED;
    Serial.println("DOOR STARTUP -> CLOSED");
  } else {
    doorState = DoorState::UNKNOWN;
    Serial.println("DOOR STARTUP -> UNKNOWN");
  }

  stateStartTime = millis();
  movementStartTime = millis();
  doorError = false;
  retryCount = 0;
  door_syncData();
}

// ---------------------------------------------------------------------------
// COMMAND OPEN
// ---------------------------------------------------------------------------

void door_open() {
  if (doorError) {
    Serial.println("DOOR ERROR - OPEN BLOCKED");
    return;
  }

  if (door_isMoving()) {
    Serial.println("DOOR ALREADY MOVING");
    return;
  }

  if (door_isTopLimitActive()) {
    door_stopOutput();
    doorState = DoorState::OPEN;
    door_syncData();
    Serial.println("DOOR ALREADY OPEN");
    return;
  }

  movementStartTime = millis();
  stateStartTime = millis();
  door_driveOpen(doorConfig.openStartPower);
  doorState = DoorState::OPENING;
  door_syncData();
  Serial.println("DOOR -> OPENING");
}

// ---------------------------------------------------------------------------
// COMMAND CLOSE
// ---------------------------------------------------------------------------

void door_close() {
  if (doorError) {
    Serial.println("DOOR ERROR - CLOSE BLOCKED");
    return;
  }

  if (door_isMoving()) {
    Serial.println("DOOR ALREADY MOVING");
    return;
  }

  if (door_isBottomLimitActive()) {
    door_stopOutput();
    doorState = DoorState::CLOSED;
    door_syncData();
    Serial.println("DOOR ALREADY CLOSED");
    return;
  }

  movementStartTime = millis();
  stateStartTime = millis();
  door_driveClose(doorConfig.closeStartPower);
  doorState = DoorState::CLOSING;
  door_syncData();
  Serial.println("DOOR -> CLOSING");
}

// ---------------------------------------------------------------------------
// STOP
// ---------------------------------------------------------------------------

void door_stop() {
  door_stopOutput();
  doorState = DoorState::STOPPED;
  door_syncData();
  Serial.println("DOOR STOPPED");
}

// ---------------------------------------------------------------------------
// ERROR RESET
// ---------------------------------------------------------------------------

void door_resetError() {
  door_stopOutput();
  doorError = false;
  retryCount = 0;

  if (door_isTopLimitActive()) {
    doorState = DoorState::OPEN;
  } else if (door_isBottomLimitActive()) {
    doorState = DoorState::CLOSED;
  } else {
    doorState = DoorState::UNKNOWN;
  }

  doorData.lastError = "";
  door_syncData();
  Serial.println("DOOR ERROR RESET");
}

// ---------------------------------------------------------------------------
// UPDATE
// ---------------------------------------------------------------------------

void door_update() {
  unsigned long now = millis();
  doorData.currentMA = sensors_getMotorCurrentMA();

  switch (doorState) {
    case DoorState::OPENING:
      if (door_isTopLimitActive()) {
        door_stopOutput();
        doorState = DoorState::OPEN;
        Serial.println("TOP LIMIT -> DOOR OPEN");
        break;
      }
      if (now - stateStartTime >= doorConfig.openStartTimeMs) {
        door_driveOpen(doorConfig.openRunPower);
        stateStartTime = now;
        doorState = DoorState::OPENING_SLOW;
        Serial.println("DOOR -> OPENING_SLOW");
      }
      break;

    case DoorState::OPENING_SLOW:
      if (door_isTopLimitActive()) {
        door_stopOutput();
        doorState = DoorState::OPEN;
        Serial.println("TOP LIMIT -> DOOR OPEN");
        break;
      }
      if (now - movementStartTime >= doorConfig.timeoutMs) {
        door_stopOutput();
        doorState = DoorState::TIMEOUT;
        doorError = true;
        doorData.lastError = "OPEN TIMEOUT";
        Serial.println("DOOR OPEN TIMEOUT");
      }
      break;

    case DoorState::CLOSING:
      if (door_isBottomLimitActive()) {
        stateStartTime = now;
        doorState = DoorState::CLOSING_SLOW;
        Serial.println("BOTTOM LIMIT -> DOOR CLOSING FINISH");
        break;
      }
      if (now - stateStartTime >= doorConfig.closeStartTimeMs) {
        door_driveClose(doorConfig.closeRunPower);
        stateStartTime = now;
        doorState = DoorState::CLOSING_SLOW;
        Serial.println("DOOR -> CLOSING_SLOW");
      }
      break;

    case DoorState::CLOSING_SLOW:
      if (door_isBottomLimitActive()) {
        if (now - stateStartTime >= doorConfig.closeExtraTimeMs) {
          door_stopOutput();
          doorState = DoorState::CLOSED;
          Serial.println("BOTTOM LIMIT -> DOOR CLOSED");
        }
        break;
      }

      if (now - movementStartTime >= doorConfig.currentIgnoreStartMs) {
        if (doorData.currentMA >= doorConfig.obstacleCurrentMA) {
          door_stopOutput();
          retryCount++;
          doorState = DoorState::OBSTACLE;
          doorError = true;
          doorData.lastError = "OBSTACLE";
          Serial.print("DOOR OBSTACLE - CURRENT: ");
          Serial.print(doorData.currentMA);
          Serial.println(" mA");
          break;
        }
      }

      if (now - movementStartTime >= doorConfig.timeoutMs) {
        door_stopOutput();
        doorState = DoorState::TIMEOUT;
        doorError = true;
        doorData.lastError = "CLOSE TIMEOUT";
        Serial.println("DOOR CLOSE TIMEOUT");
      }
      break;

    default:
      break;
  }

  door_syncData();
}

// ---------------------------------------------------------------------------
// STATE FUNCTIONS
// ---------------------------------------------------------------------------

DoorState door_getState() {
  return doorState;
}

const char* door_stateToString(DoorState state) {
  switch (state) {
    case DoorState::UNKNOWN: return "UNKNOWN";
    case DoorState::STOPPED: return "STOPPED";
    case DoorState::OPENING: return "OPENING";
    case DoorState::OPENING_SLOW: return "OPENING_SLOW";
    case DoorState::OPEN: return "OPEN";
    case DoorState::CLOSING: return "CLOSING";
    case DoorState::CLOSING_SLOW: return "CLOSING_SLOW";
    case DoorState::OBSTACLE: return "OBSTACLE";
    case DoorState::TIMEOUT: return "TIMEOUT";
    case DoorState::ERROR: return "ERROR";
    case DoorState::CLOSED: return "CLOSED";
    default: return "UNKNOWN";
  }
}

const char* door_getStateName() {
  return door_stateToString(doorState);
}

bool door_isOpen() {
  return doorState == DoorState::OPEN;
}

bool door_isClosed() {
  return doorState == DoorState::CLOSED;
}

bool door_isMoving() {
  return doorState == DoorState::OPENING ||
         doorState == DoorState::OPENING_SLOW ||
         doorState == DoorState::CLOSING ||
         doorState == DoorState::CLOSING_SLOW;
}

bool door_hasError() {
  return doorError || doorState == DoorState::OBSTACLE || doorState == DoorState::TIMEOUT || doorState == DoorState::ERROR;
}

uint16_t door_getCurrentMA() {
  return sensors_getMotorCurrentMA();
}

uint8_t door_getRetryCount() {
  return retryCount;
}

void door_startOpenReference() {
  if (!door_isTopLimitActive()) {
    door_open();
  }
}

void door_startCloseReference() {
  if (!door_isBottomLimitActive()) {
    door_close();
  }
}

void door_setConfig(const DoorConfig& cfg) {
  doorConfig = cfg;
}

const DoorConfig* door_getConfig() {
  return &doorConfig;
}

const DoorData* door_getData() {
  return &doorData;
}
