#ifndef WINDOW_H
#define WINDOW_H

#include <Arduino.h>

enum class WindowState : uint8_t {
  UNKNOWN,
  OPENING,
  OPENING_SLOW,
  OPEN,
  CLOSING,
  CLOSING_SLOW,
  CLOSED,
  OBSTACLE,
  TIMEOUT,
  STOPPED
};

struct WindowConfig {
  uint8_t openStartPower = 230;
  uint8_t openRunPower = 185;
  uint8_t closeStartPower = 250;
  uint8_t closeRunPower = 210;
  uint32_t openStartTimeMs = 150;
  uint32_t closeStartTimeMs = 250;
  uint32_t closeExtraTimeMs = 450;
  uint32_t timeoutMs = 15000;
  uint16_t obstacleCurrentMA = 1200;
  uint32_t currentIgnoreStartMs = 500;
};

void window_init();
void window_update();
void window_open();
void window_close();
void window_stop();
void window_resetError();
WindowState window_getState();
const char* window_getStateName();
bool window_isOpen();
bool window_isClosed();
bool window_isMoving();
bool window_hasError();
bool window_isTopLimitActive();
bool window_isBottomLimitActive();
uint16_t window_getCurrentMA();
uint8_t window_getRetryCount();
void window_setConfig(const WindowConfig& cfg);
const WindowConfig& window_getConfig();

#endif // WINDOW_H
