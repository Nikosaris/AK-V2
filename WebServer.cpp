#include "WebServer.h"
#include "Door.h"
#include "Window.h"
#include "Sensors.h"
#include "Heater.h"
#include "Light.h"
#include "Globals.h"
#include <WiFi.h>

WebServerData webServerData;
static WebServerConfig webServerConfig;
static WiFiServer* server = nullptr;

void webserver_init() {
  webServerData.currentState = WebServerState::STOPPED;
  webServerData.isRunning = false;
  webServerData.totalRequests = 0;
  webServerData.totalClients = 0;
  webServerData.lastRequestMs = 0;
  webServerData.hasError = false;
  webServerConfig.port = 80;
  webServerConfig.enableSSL = false;
  webServerConfig.sslPort = 443;
  webServerConfig.requireAuthentication = false;
  strcpy(webServerConfig.adminUsername, "admin");
  strcpy(webServerConfig.adminPassword, "");
  server = new WiFiServer(80);
}

bool webserver_start() {
  if (webServerData.isRunning || server == nullptr) return false;
  server->begin();
  webServerData.currentState = WebServerState::RUNNING;
  webServerData.isRunning = true;
  webServerData.totalRequests = 0;
  webServerData.totalClients = 0;
  Serial.println("Web server started on port 80");
  return true;
}

void webserver_stop() {
  if (!webServerData.isRunning || server == nullptr) return;
  server->stop();
  webServerData.currentState = WebServerState::STOPPED;
  webServerData.isRunning = false;
  Serial.println("Web server stopped");
}

const char* HTML_TEMPLATE = R"rawliteral(
<!DOCTYPE html>
<html lang="cs">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AK-V2 Kurník Kontrola</title>
</head>
<body>
  <h1>AK-V2</h1>
</body>
</html>
)rawliteral";

void webserver_update() {
  if (!webServerData.isRunning || server == nullptr) return;
  WiFiClient client = server->available();
  if (!client) return;
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
    client.println("Content-Length: " + String(strlen(HTML_TEMPLATE)));
    client.println("Connection: close");
    client.println();
    client.print(HTML_TEMPLATE);
  } else if (path == "/api/status") {
    String json = "{";
    json += "\"door_status\":\"" + String(door_getStateName()) + "\",";
    json += "\"window_status\":\"" + String(window_getStateName()) + "\",";
    json += "\"camera\":" + String(camera_isActive() ? "true" : "false") + ",";
    json += "\"coop_temp\":" + String(coopEnvironment.temperatureC, 1) + ",";
    json += "\"cabinet_temp\":" + String(cabinetEnvironment.temperatureC, 1) + ",";
    json += "\"cabinet_humidity\":" + String(cabinetEnvironment.humidityPct, 0) + ",";
    json += "\"dew_point\":" + String(cabinetEnvironment.dewPointC, 1) + ",";
    json += "\"heater\":" + String(heater_isActive() ? "true" : "false") + ",";
    json += "\"light\":" + String(light_isActive() ? "true" : "false") + ",";
    json += "\"system_mode\":\"RUN\",";
    json += "\"uptime\":" + String(systemUptime) + ",";
    json += "\"ip_address\":\"0.0.0.0\"";
    json += "}";
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(json.length()));
    client.println("Connection: close");
    client.println();
    client.print(json);
  } else if (path == "/api/door/open") {
    door_open();
    client.println("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}");
  } else if (path == "/api/door/close") {
    door_close();
    client.println("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}");
  } else if (path == "/api/window/open") {
    window_open();
    client.println("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}");
  } else if (path == "/api/window/close") {
    window_close();
    client.println("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}");
  } else if (path == "/api/camera/on") {
    camera_on();
    client.println("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}");
  } else if (path == "/api/camera/off") {
    camera_off();
    client.println("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}");
  } else if (path == "/api/heater/on") {
    heater_setMode(HeaterState::ON);
    client.println("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}");
  } else if (path == "/api/heater/off") {
    heater_setMode(HeaterState::OFF);
    client.println("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}");
  } else if (path == "/api/heater/auto") {
    heater_setMode(HeaterState::AUTO);
    client.println("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}");
  } else if (path == "/api/light/auto") {
    light_setMode(LightMode::AUTO);
    client.println("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}");
  } else if (path == "/api/light/off") {
    light_off();
    client.println("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}");
  } else if (path == "/api/restart") {
    client.println("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}");
    ESP.restart();
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.print("Not Found");
  }

  client.stop();
  webServerData.totalRequests++;
}

bool webserver_isRunning() { return webServerData.isRunning; }
WebServerState webserver_getState() { return webServerData.currentState; }
WebServerConfig* webserver_getConfig() { return &webServerConfig; }
WebServerData* webserver_getData() { return &webServerData; }
const char* webserver_getStateName(WebServerState state) {
  switch (state) {
    case WebServerState::STOPPED: return "STOPPED";
    case WebServerState::RUNNING: return "RUNNING";
    case WebServerState::ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}
void webserver_handleAPI(const char* endpoint, const char* method, const char* body) {
  (void)endpoint; (void)method; (void)body;
}
