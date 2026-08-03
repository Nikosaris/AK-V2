#include <WiFi.h>
#include <WebServer.h>

constexpr const char* AP_SSID = "AKV2-LIMITS-TEST";
constexpr const char* AP_PASSWORD = "12345678";

constexpr uint8_t DOOR_TOP_LIMIT_PIN = 16;      // active LOW
constexpr uint8_t DOOR_BOTTOM_LIMIT_PIN = 17;   // active LOW
constexpr uint8_t WINDOW_TOP_LIMIT_PIN = 36;    // active HIGH
constexpr uint8_t WINDOW_BOTTOM_LIMIT_PIN = 39; // active HIGH

WebServer server(80);

static String statusJson() {
  bool doorTop = digitalRead(DOOR_TOP_LIMIT_PIN) == LOW;
  bool doorBottom = digitalRead(DOOR_BOTTOM_LIMIT_PIN) == LOW;
  bool windowTop = digitalRead(WINDOW_TOP_LIMIT_PIN) == HIGH;
  bool windowBottom = digitalRead(WINDOW_BOTTOM_LIMIT_PIN) == HIGH;

  String json = "{";
  json += "\"door_top\":" + String(doorTop ? "true" : "false") + ",";
  json += "\"door_bottom\":" + String(doorBottom ? "true" : "false") + ",";
  json += "\"window_top\":" + String(windowTop ? "true" : "false") + ",";
  json += "\"window_bottom\":" + String(windowBottom ? "true" : "false");
  json += "}";
  return json;
}

static const char* HTML = R"html(
<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>AK-V2 Limit Test</title><style>body{font-family:Arial;background:#111;color:#eee;padding:16px}pre{background:#222;padding:12px}</style></head>
<body><h2>AK-V2: Test koncových čidel</h2><p>Kontrola stavů dveře/ventilace.</p><pre id='out'>--</pre>
<script>
async function load(){const r=await fetch('/api/status');document.getElementById('out').textContent=JSON.stringify(await r.json(),null,2)}
setInterval(load,700);load();
</script></body></html>
)html";

void setup() {
  Serial.begin(115200);
  pinMode(DOOR_TOP_LIMIT_PIN, INPUT_PULLUP);
  pinMode(DOOR_BOTTOM_LIMIT_PIN, INPUT_PULLUP);
  pinMode(WINDOW_TOP_LIMIT_PIN, INPUT);
  pinMode(WINDOW_BOTTOM_LIMIT_PIN, INPUT);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  server.on("/", []() { server.send(200, "text/html", HTML); });
  server.on("/api/status", []() { server.send(200, "application/json", statusJson()); });
  server.begin();

  Serial.print("[LIMITS] AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() { server.handleClient(); }
