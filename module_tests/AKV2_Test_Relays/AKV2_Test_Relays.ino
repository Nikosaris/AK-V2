#include <WiFi.h>
#include <WebServer.h>

constexpr const char* AP_SSID = "AKV2-RELAYS-TEST";
constexpr const char* AP_PASSWORD = "12345678";

constexpr uint8_t CAMERA_RELAY_PIN = 26;
constexpr uint8_t HEATER_RELAY_PIN = 27;
constexpr uint8_t LIGHT_RELAY_PIN = 14;

WebServer server(80);

static void setRelay(uint8_t pin, bool on) { digitalWrite(pin, on ? HIGH : LOW); }
static bool getRelay(uint8_t pin) { return digitalRead(pin) == HIGH; }

static String statusJson() {
  String json = "{";
  json += "\"camera\":" + String(getRelay(CAMERA_RELAY_PIN) ? "true" : "false") + ",";
  json += "\"heater\":" + String(getRelay(HEATER_RELAY_PIN) ? "true" : "false") + ",";
  json += "\"light\":" + String(getRelay(LIGHT_RELAY_PIN) ? "true" : "false");
  json += "}";
  return json;
}

static const char* HTML = R"html(
<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>AK-V2 Relay Test</title><style>body{font-family:Arial;background:#111;color:#eee;padding:16px}button{padding:12px;margin:4px;min-width:90px}pre{background:#222;padding:12px}</style></head>
<body><h2>AK-V2: Test relé</h2><p>Pouze ruční ON/OFF bez logiky.</p>
<div>
<button onclick="api('/api/camera/on')">Kamera ON</button><button onclick="api('/api/camera/off')">Kamera OFF</button><br>
<button onclick="api('/api/heater/on')">Topení ON</button><button onclick="api('/api/heater/off')">Topení OFF</button><br>
<button onclick="api('/api/light/on')">Světlo ON</button><button onclick="api('/api/light/off')">Světlo OFF</button>
</div><pre id='out'>--</pre>
<script>
async function api(u){await fetch(u);load()}
async function load(){const r=await fetch('/api/status');document.getElementById('out').textContent=JSON.stringify(await r.json(),null,2)}
setInterval(load,1000);load();
</script></body></html>
)html";

void setup() {
  Serial.begin(115200);
  pinMode(CAMERA_RELAY_PIN, OUTPUT);
  pinMode(HEATER_RELAY_PIN, OUTPUT);
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  setRelay(CAMERA_RELAY_PIN, false);
  setRelay(HEATER_RELAY_PIN, false);
  setRelay(LIGHT_RELAY_PIN, false);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  server.on("/", []() { server.send(200, "text/html", HTML); });
  server.on("/api/status", []() { server.send(200, "application/json", statusJson()); });
  server.on("/api/camera/on", []() { setRelay(CAMERA_RELAY_PIN, true); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/camera/off", []() { setRelay(CAMERA_RELAY_PIN, false); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/heater/on", []() { setRelay(HEATER_RELAY_PIN, true); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/heater/off", []() { setRelay(HEATER_RELAY_PIN, false); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/light/on", []() { setRelay(LIGHT_RELAY_PIN, true); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/light/off", []() { setRelay(LIGHT_RELAY_PIN, false); server.send(200, "application/json", "{\"ok\":true}"); });
  server.begin();

  Serial.print("[RELAYS] AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() { server.handleClient(); }
