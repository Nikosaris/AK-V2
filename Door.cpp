#include "Door.h"
#include "Globals.h"
#include "Sensors.h"

static DoorState doorState = DoorState::UNKNOWN;
static DoorConfig doorConfig;
static uint8_t retryCount = 0;
static bool doorError = false;

bool door_isTopLimitActive() { return digitalRead(DOOR_TOP_LIMIT_PIN); }
bool door_isBottomLimitActive() { return digitalRead(DOOR_BOTTOM_LIMIT_PIN); }

void door_init() {
  pinMode(DOOR_TOP_LIMIT_PIN, INPUT);
  pinMode(DOOR_BOTTOM_LIMIT_PIN, INPUT);
  ledcAttach(DOOR_IN1_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttach(DOOR_IN2_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
  doorState = DoorState::UNKNOWN;
}

void door_open() {
  if (doorError || door_isMoving()) return;
  doorState = DoorState::OPENING;
  ledcWrite(DOOR_IN1_PIN, doorConfig.openStartPower);
  ledcWrite(DOOR_IN2_PIN, 0);
}

void door_close() {
  if (doorError || door_isMoving()) return;
  doorState = DoorState::CLOSING;
  ledcWrite(DOOR_IN1_PIN, 0);
  ledcWrite(DOOR_IN2_PIN, doorConfig.closeStartPower);
}

void door_stop() {
  ledcWrite(DOOR_IN1_PIN, 0);
  ledcWrite(DOOR_IN2_PIN, 0);
  doorState = DoorState::STOPPED;
}

void door_resetError() {
  doorError = false;
  retryCount = 0;
}

void door_update() {
  if (doorState == DoorState::OPENING && door_isTopLimitActive()) { door_stop(); doorState = DoorState::OPEN; }
  if (doorState == DoorState::CLOSING && door_isBottomLimitActive()) { door_stop(); doorState = DoorState::CLOSED; }
}

DoorState door_getState() { return doorState; }
const char* door_getStateName() {
  switch (doorState) {
    case DoorState::UNKNOWN: return "UNKNOWN";
    case DoorState::OPENING: return "OPENING";
    case DoorState::OPENING_SLOW: return "OPENING_SLOW";
    case DoorState::OPEN: return "OPEN";
    case DoorState::CLOSING: return "CLOSING";
    case DoorState::CLOSING_SLOW: return "CLOSING_SLOW";
    case DoorState::CLOSED: return "CLOSED";
    case DoorState::OBSTACLE: return "OBSTACLE";
    case DoorState::TIMEOUT: return "TIMEOUT";
    case DoorState::STOPPED: return "STOPPED";
    default: return "UNKNOWN";
  }
}
bool door_isOpen() { return doorState == DoorState::OPEN; }
bool door_isClosed() { return doorState == DoorState::CLOSED; }
bool door_isMoving() { return doorState == DoorState::OPENING || doorState == DoorState::OPENING_SLOW || doorState == DoorState::CLOSING || doorState == DoorState::CLOSING_SLOW; }
bool door_hasError() { return doorError || doorState == DoorState::OBSTACLE || doorState == DoorState::TIMEOUT; }
uint16_t door_getCurrentMA() { return sensors_getMotorCurrentMA(); }
uint8_t door_getRetryCount() { return retryCount; }
void door_startOpenReference() { door_open(); }
void door_startCloseReference() { door_close(); }
void door_setConfig(const DoorConfig& cfg) { doorConfig = cfg; }
const DoorConfig& door_getConfig() { return doorConfig; }
