#include "Window.h"
#include "Globals.h"
#include "Sensors.h"

static WindowState windowState = WindowState::UNKNOWN;
static WindowConfig windowConfig;
static uint8_t retryCount = 0;
static bool windowError = false;

bool window_isTopLimitActive() { return digitalRead(WINDOW_TOP_LIMIT_PIN); }
bool window_isBottomLimitActive() { return digitalRead(WINDOW_BOTTOM_LIMIT_PIN); }

void window_init() {
  pinMode(WINDOW_TOP_LIMIT_PIN, INPUT);
  pinMode(WINDOW_BOTTOM_LIMIT_PIN, INPUT);
  ledcAttach(WINDOW_IN1_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttach(WINDOW_IN2_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
  windowState = WindowState::UNKNOWN;
}

void window_open() {
  if (windowError || window_isMoving()) return;
  windowState = WindowState::OPENING;
  ledcWrite(WINDOW_IN1_PIN, windowConfig.openStartPower);
  ledcWrite(WINDOW_IN2_PIN, 0);
}

void window_close() {
  if (windowError || window_isMoving()) return;
  windowState = WindowState::CLOSING;
  ledcWrite(WINDOW_IN1_PIN, 0);
  ledcWrite(WINDOW_IN2_PIN, windowConfig.closeStartPower);
}

void window_stop() {
  ledcWrite(WINDOW_IN1_PIN, 0);
  ledcWrite(WINDOW_IN2_PIN, 0);
  windowState = WindowState::STOPPED;
}

void window_resetError() {
  windowError = false;
  retryCount = 0;
}

void window_update() {
  if (windowState == WindowState::OPENING && window_isTopLimitActive()) { window_stop(); windowState = WindowState::OPEN; }
  if (windowState == WindowState::CLOSING && window_isBottomLimitActive()) { window_stop(); windowState = WindowState::CLOSED; }
}

WindowState window_getState() { return windowState; }
const char* window_getStateName() {
  switch (windowState) {
    case WindowState::UNKNOWN: return "UNKNOWN";
    case WindowState::OPENING: return "OPENING";
    case WindowState::OPENING_SLOW: return "OPENING_SLOW";
    case WindowState::OPEN: return "OPEN";
    case WindowState::CLOSING: return "CLOSING";
    case WindowState::CLOSING_SLOW: return "CLOSING_SLOW";
    case WindowState::CLOSED: return "CLOSED";
    case WindowState::OBSTACLE: return "OBSTACLE";
    case WindowState::TIMEOUT: return "TIMEOUT";
    case WindowState::STOPPED: return "STOPPED";
    default: return "UNKNOWN";
  }
}
bool window_isOpen() { return windowState == WindowState::OPEN; }
bool window_isClosed() { return windowState == WindowState::CLOSED; }
bool window_isMoving() { return windowState == WindowState::OPENING || windowState == WindowState::OPENING_SLOW || windowState == WindowState::CLOSING || windowState == WindowState::CLOSING_SLOW; }
bool window_hasError() { return windowError || windowState == WindowState::OBSTACLE || windowState == WindowState::TIMEOUT; }
uint16_t window_getCurrentMA() { return sensors_getMotorCurrentMA(); }
uint8_t window_getRetryCount() { return retryCount; }
void window_setConfig(const WindowConfig& cfg) { windowConfig = cfg; }
const WindowConfig& window_getConfig() { return windowConfig; }
