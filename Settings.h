#ifndef SETTINGS_H
#define SETTINGS_H

#include "Globals.h"
#include "Door.h"
#include "Window.h"

// ============================================================================
// SETTINGS MANAGEMENT
// ============================================================================

void settings_init();
void settings_load();
void settings_save();
void settings_reset();

DoorConfig* settings_getDoorConfig();
WindowConfig* settings_getWindowConfig();
void settings_applyDoorConfig(const DoorConfig& cfg);
void settings_applyWindowConfig(const WindowConfig& cfg);

#endif // SETTINGS_H
