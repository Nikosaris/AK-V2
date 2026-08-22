#include "Window.h"

#include "Globals.h"
#include "Sensors.h"

constexpr uint8_t WINDOW_TOP_LIMIT = WINDOW_TOP_LIMIT_PIN;
constexpr uint8_t WINDOW_BOTTOM_LIMIT = WINDOW_BOTTOM_LIMIT_PIN;

static WindowState windowState = WindowState::UNKNOWN;
static WindowConfig windowConfig;
static WindowData windowData;

static unsigned long stateStartTime = 0;
static unsigned long movementStartTime = 0;
static bool windowError = false;
static uint8_t retryCount = 0;

static void window_syncData() {
  windowData.state = windowState;
  windowData.isOpen = (windowState == WindowState::OPEN);
  windowData.isClosed = (windowState == WindowState::CLOSED);
  windowData.hasError = windowError || windowState == WindowState::OBSTACLE || windowState == WindowState::TIMEOUT || windowState == WindowState::ERROR;
  windowData.isMoving = window_isMoving();
  windowData.currentMA = sensors_getMotorCurrentMA();
  windowData.retryCount = retryCount;
  windowData.stateStartMs = stateStartTime;
  windowData.movementStartMs = movementStartTime;
}

static void window_stopOutput() {
  ledcWrite(WINDOW_IN1_PIN, 0);
  ledcWrite(WINDOW_IN2_PIN, 0);
}

static void window_driveOpen(uint8_t power) {
  ledcWrite(WINDOW_IN1_PIN, power);
  ledcWrite(WINDOW_IN2_PIN, 0);
}

static void window_driveClose(uint8_t power) {
  ledcWrite(WINDOW_IN1_PIN, 0);
  ledcWrite(WINDOW_IN2_PIN, power);
}

bool window_isTopLimitActive() {
  return digitalRead(WINDOW_TOP_LIMIT);
}

bool window_isBottomLimitActive() {
  return digitalRead(WINDOW_BOTTOM_LIMIT);
}

void window_init() {
  pinMode(WINDOW_TOP_LIMIT, INPUT);
  pinMode(WINDOW_BOTTOM_LIMIT, INPUT);

  ledcAttach(WINDOW_IN1_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttach(WINDOW_IN2_PIN, PWM_FREQUENCY, PWM_RESOLUTION);

  window_stopOutput();

  if (window_isTopLimitActive()) {
    windowState = WindowState::OPEN;
    Serial.println("WINDOW STARTUP -> OPEN");
  } else if (window_isBottomLimitActive()) {
    windowState = WindowState::CLOSED;
    Serial.println("WINDOW STARTUP -> CLOSED");
  } else {
    windowState = WindowState::UNKNOWN;
    Serial.println("WINDOW STARTUP -> UNKNOWN");
  }

  stateStartTime = millis();
  movementStartTime = millis();
  windowError = false;
  retryCount = 0;
  window_syncData();
}

void window_open() {
  if (windowError) {
    Serial.println("WINDOW ERROR - OPEN BLOCKED");
    return;
  }
  if (window_isMoving()) {
    Serial.println("WINDOW ALREADY MOVING");
    return;
  }
  if (window_isTopLimitActive()) {
    window_stopOutput();
    windowState = WindowState::OPEN;
    window_syncData();
    Serial.println("WINDOW ALREADY OPEN");
    return;
  }

  movementStartTime = millis();
  stateStartTime = millis();
  window_driveOpen(windowConfig.openStartPower);
  windowState = WindowState::OPENING;
  window_syncData();
  Serial.println("WINDOW -> OPENING");
}

void window_close() {
  if (windowError) {
    Serial.println("WINDOW ERROR - CLOSE BLOCKED");
    return;
  }
  if (window_isMoving()) {
    Serial.println("WINDOW ALREADY MOVING");
    return;
  }
  if (window_isBottomLimitActive()) {
    window_stopOutput();
    windowState = WindowState::CLOSED;
    window_syncData();
    Serial.println("WINDOW ALREADY CLOSED");
    return;
  }

  movementStartTime = millis();
  stateStartTime = millis();
  window_driveClose(windowConfig.closeStartPower);
  windowState = WindowState::CLOSING;
  window_syncData();
  Serial.println("WINDOW -> CLOSING");
}

void window_stop() {
  window_stopOutput();
  windowState = WindowState::STOPPED;
  window_syncData();
  Serial.println("WINDOW STOPPED");
}

void window_resetError() {
  window_stopOutput();
  windowError = false;
  retryCount = 0;
  if (window_isTopLimitActive()) windowState = WindowState::OPEN;
  else if (window_isBottomLimitActive()) windowState = WindowState::CLOSED;
  else windowState = WindowState::UNKNOWN;
  windowData.lastError = "";
  window_syncData();
  Serial.println("WINDOW ERROR RESET");
}

void window_update() {
  unsigned long now = millis();
  windowData.currentMA = sensors_getMotorCurrentMA();

  switch (windowState) {
    case WindowState::OPENING:
      if (window_isTopLimitActive()) {
        window_stopOutput();
        windowState = WindowState::OPEN;
        Serial.println("TOP LIMIT -> WINDOW OPEN");
        break;
      }
      if (now - stateStartTime >= windowConfig.openStartTimeMs) {
        window_driveOpen(windowConfig.openRunPower);
        stateStartTime = now;
        windowState = WindowState::OPENING_SLOW;
        Serial.println("WINDOW -> OPENING_SLOW");
      }
      break;

    case WindowState::OPENING_SLOW:
      if (window_isTopLimitActive()) {
        window_stopOutput();
        windowState = WindowState::OPEN;
        Serial.println("TOP LIMIT -> WINDOW OPEN");
        break;
      }
      if (now - movementStartTime >= windowConfig.timeoutMs) {
        window_stopOutput();
        windowState = WindowState::TIMEOUT;
        windowError = true;
        windowData.lastError = "OPEN TIMEOUT";
        Serial.println("WINDOW OPEN TIMEOUT");
      }
      break;

    case WindowState::CLOSING:
      if (window_isBottomLimitActive()) {
        stateStartTime = now;
        windowState = WindowState::CLOSING_SLOW;
        Serial.println("BOTTOM LIMIT -> WINDOW FINISH");
        break;
      }
      if (now - stateStartTime >= windowConfig.closeStartTimeMs) {
        window_driveClose(windowConfig.closeRunPower);
        stateStartTime = now;
        windowState = WindowState::CLOSING_SLOW;
        Serial.println("WINDOW -> CLOSING_SLOW");
      }
      break;

    case WindowState::CLOSING_SLOW:
      if (window_isBottomLimitActive()) {
        if (now - stateStartTime >= windowConfig.closeExtraTimeMs) {
          window_stopOutput();
          windowState = WindowState::CLOSED;
          Serial.println("BOTTOM LIMIT -> WINDOW CLOSED");
        }
        break;
      }
      if (now - movementStartTime >= windowConfig.currentIgnoreStartMs) {
        if (windowData.currentMA >= windowConfig.obstacleCurrentMA) {
          window_stopOutput();
          retryCount++;
          windowState = WindowState::OBSTACLE;
          windowError = true;
          windowData.lastError = "OBSTACLE";
          Serial.println("WINDOW OBSTACLE");
          break;
        }
      }
      if (now - movementStartTime >= windowConfig.timeoutMs) {
        window_stopOutput();
        windowState = WindowState::TIMEOUT;
        windowError = true;
        windowData.lastError = "CLOSE TIMEOUT";
        Serial.println("WINDOW CLOSE TIMEOUT");
      }
      break;

    default:
      break;
  }

  window_syncData();
}

WindowState window_getState() { return windowState; }

const char* window_stateToString(WindowState state) {
  switch (state) {
    case WindowState::UNKNOWN: return "UNKNOWN";
    case WindowState::STOPPED: return "STOPPED";
    case WindowState::OPENING: return "OPENING";
    case WindowState::OPENING_SLOW: return "OPENING_SLOW";
    case WindowState::OPEN: return "OPEN";
    case WindowState::CLOSING: return "CLOSING";
    case WindowState::CLOSING_SLOW: return "CLOSING_SLOW";
    case WindowState::OBSTACLE: return "OBSTACLE";
    case WindowState::TIMEOUT: return "TIMEOUT";
    case WindowState::ERROR: return "ERROR";
    case WindowState::CLOSED: return "CLOSED";
    default: return "UNKNOWN";
  }
}

const char* window_getStateName() { return window_stateToString(windowState); }

bool window_isOpen() { return windowState == WindowState::OPEN; }
bool window_isClosed() { return windowState == WindowState::CLOSED; }
bool window_isMoving() {
  return windowState == WindowState::OPENING || windowState == WindowState::OPENING_SLOW || windowState == WindowState::CLOSING || windowState == WindowState::CLOSING_SLOW;
}
bool window_hasError() { return windowError || windowState == WindowState::OBSTACLE || windowState == WindowState::TIMEOUT || windowState == WindowState::ERROR; }
uint16_t window_getCurrentMA() { return sensors_getMotorCurrentMA(); }
uint8_t window_getRetryCount() { return retryCount; }

void window_startOpenReference() { if (!window_isTopLimitActive()) window_open(); }
void window_startCloseReference() { if (!window_isBottomLimitActive()) window_close(); }

void window_setConfig(const WindowConfig& cfg) { windowConfig = cfg; }
const WindowConfig* window_getConfig() { return &windowConfig; }
const WindowData* window_getData() { return &windowData; }
