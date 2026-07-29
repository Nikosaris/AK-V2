#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include "Globals.h"

// ============================================================================
// WEB SERVER STATE ENUM
// ============================================================================

enum class WebServerState : uint8_t {
  STOPPED = 0,        // Server not running
  RUNNING = 1,        // Server running
  ERROR = 2           // Server error
};

// ============================================================================
// WEB SERVER CONFIGURATION
// ============================================================================

struct WebServerConfig {
  uint16_t port = 80;                    // HTTP port
  bool enableSSL = false;                // HTTPS support (requires certificate)
  uint16_t sslPort = 443;                // HTTPS port
  bool requireAuthentication = false;    // Require login
  char adminUsername[32] = "admin";
  char adminPassword[64] = "";
};

// ============================================================================
// WEB SERVER DATA - RUNTIME STATE
// ============================================================================

struct WebServerData {
  WebServerState currentState = WebServerState::STOPPED;
  bool isRunning = false;
  uint32_t totalRequests = 0;            // Total HTTP requests served
  uint32_t totalClients = 0;             // Total unique clients
  unsigned long lastRequestMs = 0;       // When last request was served
  bool hasError = false;
};

// ============================================================================
// WEB SERVER INSTANCE
// ============================================================================

extern WebServerData webServerData;

// ============================================================================
// WEB SERVER CONTROL FUNCTIONS
// ============================================================================

/**
 * Initialize web server
 */
void webserver_init();

/**
 * Start web server
 * @return - true if server started successfully
 */
bool webserver_start();

/**
 * Stop web server
 */
void webserver_stop();

/**
 * Update web server (handle client requests)
 * Should be called frequently in main loop
 */
void webserver_update();

/**
 * Check if web server is running
 */
bool webserver_isRunning();

/**
 * Get current web server state
 */
WebServerState webserver_getState();

/**
 * Get web server configuration
 */
WebServerConfig* webserver_getConfig();

/**
 * Get web server data
 */
WebServerData* webserver_getData();

/**
 * Handle API request (called by web server)
 */
void webserver_handleAPI(const char* endpoint, const char* method, const char* body);

/**
 * Get human-readable state name
 */
const char* webserver_getStateName(WebServerState state);

#endif // WEBSERVER_H
