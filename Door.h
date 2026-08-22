#ifndef DOOR_H
#define DOOR_H

#include <Arduino.h>

enum class DoorState : uint8_t {
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

struct DoorConfig {
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

struct DoorData {
  DoorState state = DoorState::UNKNOWN;
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

void door_init();
void door_update();

void door_open();
void door_close();
void door_stop();
void door_resetError();

DoorState door_getState();
const char* door_getStateName();
const char* door_stateToString(DoorState state);

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
const DoorConfig* door_getConfig();

const DoorData* door_getData();

#endif // DOOR_H
