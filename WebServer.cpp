#include "WebServer.h"
#include "Door.h"
#include "Window.h"
#include "Sensors.h"
#include "Heater.h"
#include "Light.h"
#include "Globals.h"
#include <WiFi.h>

// ============================================================================
// WEB SERVER INSTANCE - GLOBAL DATA
// ============================================================================

WebServerData webServerData;
static WebServerConfig webServerConfig;
static WiFiServer* server = nullptr;

// ============================================================================
// INITIALIZATION
// ============================================================================

void webserver_init() {
  webServerData.currentState = WebServerState::STOPPED;
  webServerData.isRunning = false;
  webServerData.totalRequests = 0;
  webServerData.totalClients = 0;
  webServerData.lastRequestMs = 0;
  webServerData.hasError = false;

  // Initialize config
  webServerConfig.port = 80;
  webServerConfig.enableSSL = false;
  webServerConfig.sslPort = 443;
  webServerConfig.requireAuthentication = false;
  strcpy(webServerConfig.adminUsername, "admin");
  strcpy(webServerConfig.adminPassword, "");

  // Create web server instance
  server = new WiFiServer(80);
}

// ============================================================================
// WEB SERVER CONTROL
// ============================================================================

bool webserver_start() {
  if (webServerData.isRunning || server == nullptr) {
    return false;
  }

  server->begin();

  webServerData.currentState = WebServerState::RUNNING;
  webServerData.isRunning = true;
  webServerData.totalRequests = 0;
  webServerData.totalClients = 0;

  Serial.println("Web server started on port 80");

  return true;
}

void webserver_stop() {
  if (!webServerData.isRunning || server == nullptr) {
    return;
  }

  server->stop();

  webServerData.currentState = WebServerState::STOPPED;
  webServerData.isRunning = false;

  Serial.println("Web server stopped");
}

// ============================================================================
// HTML TEMPLATE
// ============================================================================

const char* HTML_TEMPLATE = R"rawliteral(
<!DOCTYPE html>
<html lang="cs">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AK-V2 Kurník Kontrola</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: #1a1a1a;
      color: #e0e0e0;
      overflow-x: hidden;
    }
    
    .container {
      display: flex;
      min-height: 100vh;
    }
    
    /* SIDEBAR */
    .sidebar {
      width: 250px;
      background: #0d0d0d;
      padding: 20px;
      border-right: 1px solid #333;
      overflow-y: auto;
      position: fixed;
      height: 100vh;
      left: 0;
      top: 0;
      z-index: 1000;
    }
    
    .logo {
      font-size: 20px;
      font-weight: bold;
      color: #4CAF50;
      margin-bottom: 30px;
      text-align: center;
    }
    
    .menu-item {
      padding: 12px;
      margin: 5px 0;
      border-radius: 5px;
      cursor: pointer;
