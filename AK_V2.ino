#include "Globals.h"
#include "Hardware.h"
#include "Door.h"
#include "Window.h"
#include "Settings.h"

Door doorMotor;
Window windowMotor;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(100);

  globals_init();
  hardware_init();
  settings_init();

  door_init();
  door_setConfig(*settings_getDoorConfig());
  window_init();
  window_setConfig(*settings_getWindowConfig());
}

void loop() {
  const unsigned long loopStartTime = millis();

  globals_update();
  hardware_update();
  door_update();
  window_update();

  const unsigned long currentTime = millis();
  if (currentTime - lastSerialLogTime >= SERIAL_LOG_INTERVAL_MS) {
    lastSerialLogTime = currentTime;
    Serial.print("[LOG] Door: ");
    Serial.print(door_getStateName());
    Serial.print(" | Window: ");
    Serial.println(window_getStateName());
  }

  const unsigned long loopDuration = millis() - loopStartTime;
  if (loopDuration < LOOP_INTERVAL_MS) {
    delayMicroseconds((LOOP_INTERVAL_MS - loopDuration) * 1000);
  }
}
