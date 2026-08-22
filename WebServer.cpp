#include "WebServer.h"
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

void webserver_update() {
  if (!webServerData.isRunning || server == nullptr) {
    return;
  }

  WiFiClient client = server->available();
  if (!client) {
    return;
  }

  unsigned long timeout = millis() + 1000;
  while (client.connected() && !client.available() && millis() < timeout) {
    delay(1);
  }

  if (!client.available()) {
    client.stop();
    return;
  }

  String request = client.readStringUntil('\n');
  request.trim();

  String method = request.substring(0, request.indexOf(' '));
  String path = request.substring(request.indexOf(' ') + 1, request.lastIndexOf(' '));

  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  if (path == "/" || path == "/index.html") {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("Content-Length: 0");
    client.println("Connection: close");
    client.println();
  } else if (path == "/api/status") {
    String json = "{";
    json += "\"door_status\":\"ZAVŘENO\",";
    json += "\"window_status\":\"ZAVŘENO\",";
    json += "\"camera\":false,";
    json += "\"coop_temp\":22.5,";
    json += "\"cabinet_temp\":18.3,";
    json += "\"cabinet_humidity\":65,";
    json += "\"dew_point\":11.2,";
    json += "\"heater\":false,";
    json += "\"light\":false,";
    json += "\"system_mode\":\"RUN\",";
    json += "\"uptime\":141000,";
    json += "\"ip_address\":\"172.20.10.6\"";
    json += "}";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(json.length()));
    client.println("Connection: close");
    client.println();
    client.print(json);
  } else if (path.startsWith("/api/")) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Content-Length: 16");
    client.println("Connection: close");
    client.println();
    client.print("{\"status\":\"ok\"}");
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/html");
    client.println("Content-Length: 9");
    client.println("Connection: close");
    client.println();
    client.print("Not Found");
  }

  delay(1);
  client.stop();

  webServerData.totalRequests++;
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool webserver_isRunning() {
  return webServerData.isRunning;
}

WebServerState webserver_getState() {
  return webServerData.currentState;
}

WebServerConfig* webserver_getConfig() {
  return &webServerConfig;
}

WebServerData* webserver_getData() {
  return &webServerData;
}

const char* webserver_getStateName(WebServerState state) {
  switch (state) {
    case WebServerState::STOPPED: return "STOPPED";
    case WebServerState::RUNNING: return "RUNNING";
    case WebServerState::ERROR:   return "ERROR";
    default:                      return "UNKNOWN";
  }
}
