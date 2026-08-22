#ifndef DOOR_H
#define DOOR_H

#include <Arduino.h>

enum class DoorState : uint8_t {
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

struct DoorConfig {
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

void door_init();
void door_update();
void door_open();
void door_close();
void door_stop();
void door_resetError();
DoorState door_getState();
const char* door_getStateName();
bool door_isOpen();
bool door_isClosed();
bool door_isMoving();
bool door_hasError();
bool door_isTopLimitActive();
bool door_isBottomLimitActive();
uint16_t door_getCurrentMA();
uint8_t door_getRetryCount();
void door_startOpenReference();
void door_startCloseReference();
void door_setConfig(const DoorConfig& cfg);
const DoorConfig& door_getConfig();

#endif // DOOR_H
