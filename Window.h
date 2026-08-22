#ifndef WINDOW_H
#define WINDOW_H

#include <Arduino.h>

enum class WindowState : uint8_t {
  UNKNOWN = 0,
  STOPPED = 1,
  OPENING = 2,
  OPENING_SLOW = 3,
  OPEN = 4,
  CLOSING = 5,
  CLOSING_SLOW = 6,
  OBSTACLE = 7,
  TIMEOUT = 8,
  ERROR = 9,
  CLOSED = 10
};

struct WindowConfig {
  uint16_t timeoutMs = 15000;
  uint8_t openStartPower = 230;
  uint8_t openRunPower = 185;
  uint8_t closeStartPower = 250;
  uint8_t closeRunPower = 210;
  uint8_t slowPower = 120;
  uint32_t openStartTimeMs = 150;
  uint32_t closeStartTimeMs = 250;
  uint32_t closeExtraTimeMs = 450;
  uint16_t obstacleCurrentMA = 1200;
  uint32_t currentIgnoreStartMs = 500;
  uint8_t maxRetries = 3;
};

struct WindowData {
  WindowState state = WindowState::UNKNOWN;
  bool isOpen = false;
  bool isClosed = false;
  bool hasError = false;
  bool isMoving = false;
  uint16_t currentMA = 0;
  uint8_t retryCount = 0;
  uint32_t stateStartMs = 0;
  uint32_t movementStartMs = 0;
  uint32_t currentHighStartMs = 0;
  const char* lastError = "";
};

void window_init();
void window_update();

void window_open();
void window_close();
void window_stop();
void window_resetError();

WindowState window_getState();
const char* window_getStateName();
const char* window_stateToString(WindowState state);

bool window_isOpen();
bool window_isClosed();
bool window_isMoving();
bool window_hasError();

bool window_isTopLimitActive();
bool window_isBottomLimitActive();

uint16_t window_getCurrentMA();
uint8_t window_getRetryCount();

void window_startOpenReference();
void window_startCloseReference();

void window_setConfig(const WindowConfig& cfg);
const WindowConfig* window_getConfig();

const WindowData* window_getData();

#endif // WINDOW_H
