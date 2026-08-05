#ifndef ETHERNETNTP_H
#define ETHERNETNTP_H

#include <Arduino.h>
#include "Globals.h"
#include "RTC.h"

// ============================================================================
// ETHERNET W5500 + NTP
// ============================================================================

// NTP re-sync interval (24 hours in milliseconds)
#ifndef NTP_SYNC_INTERVAL_MS
constexpr uint32_t NTP_SYNC_INTERVAL_MS = 86400000UL;
#endif

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * Initialize Ethernet W5500 module (SPI, DHCP).
 */
void ethernet_init();

/**
 * Update Ethernet state machine (call every loop).
 * Handles DHCP renewal and periodic NTP sync.
 */
void ethernet_update();

/**
 * Check if Ethernet is connected (link up + IP obtained).
 */
bool ethernet_isConnected();

/**
 * Perform a single NTP synchronisation over Ethernet.
 * Calls rtc_syncFromNTP(TimeData*) with the received time.
 * @return true if sync succeeded
 */
bool ethernet_ntpSync();

/**
 * Perform NTP sync with fallback retry logic.
 * @return true if at least one sync succeeded
 */
bool ethernet_ntpSyncWithFallback();

/**
 * Get local IP address as string.
 */
const char* ethernet_getLocalIP();

#endif // ETHERNETNTP_H
