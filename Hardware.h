#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include <stdint.h>
#include "Globals.h"

// ============================================================================
// HARDWARE INITIALIZATION AND PIN CONFIGURATION
// ============================================================================

/**
 * Initialize all hardware pins and peripherals
 * - Set GPIO pin modes
 * - Configure PWM channels
 * - Initialize I2C and OneWire buses
 * - Setup analog-to-digital converter
 */
void hardware_init();

/**
 * Update hardware state (reading inputs, managing I/O)
 * Should be called in main loop
 */
void hardware_update();

/**
 * Shutdown all hardware safely
 */
void hardware_shutdown();

#endif // HARDWARE_H
