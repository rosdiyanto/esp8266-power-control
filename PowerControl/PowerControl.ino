// ======================================================
// ESP8266 POWER CONTROL - FINAL FULL VERSION
// WS + LOG FILE + HISTORY + WIFI LED + /log FIXED
// ======================================================

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <LittleFS.h>

// ================= WIFI =================
const char *ssid = "NamaWiFiAnda";
const char *password = "PasswordWiFiAnda";

// ================= SERVER =================
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

// ================= PIN =================
#define POWER_PIN 5
#define STATUS_PIN 4
#define LED_PIN LED_BUILTIN

// ================= STATE =================
bool lastStatus = false;
unsigned long lastChange = 0;

bool wifiWasConnected = false;
unsigned long lastReconnectAttempt = 0;
int retryDelay = 20000;

// ======================================================
// LED WIFI STATUS
// ======================================================
void updateWiFiLED() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, LOW);   // ON (active LOW)
  } else {
    digitalWrite(LED_PIN, HIGH);  // OFF
  }
}

// ======================================================
// SEND LOG VIA WS
// ======================================================
void sendLog(String msg) {
  // Menggunakan char array untuk menghindari fragmentasi memori
  char jsonBuffer[msg.length() + 20];
  snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"log\":\"%s\"}", msg.c_str());
  webSocket.broadcastTXT(jsonBuffer);
}

// ======================================================
// WRITE LOG FILE
// ======================================================
void writeLog(String msg) {

  File file = LittleFS.open("/log.txt", "a");
  if (!file) return;

  if (file.size() > 100000) {
    file.close();
    LittleFS.remove("/log.txt");
    file = LittleFS.open("/log.txt", "a");
  }

  String line = String(millis()) + " | " + msg;

  file.println(line);
  file.close();

  Serial.println(line);
  sendLog(line);
}

// ======================================================
// HISTORY LOG (FOR WS CLIENT)
// ======================================================
void sendLogHistory(uint8_t client) {

  File file = LittleFS.open("/log.txt", "r");
  if (!file) return;

  while (file.available()) {

    String line = file.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) continue;

    char jsonBuffer[line.length() + 20];
    snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"log\":\"%s\"}", line.c_str());
    webSocket.sendTXT(client, jsonBuffer);
  }

  file.close();
}

// ======================================================
// WEB HANDLERS
// ======================================================
void handleRoot() {
  File file = LittleFS.open("/index.html", "r");

  if (!file) {
    server.send(404, "text/plain", "NO INDEX");
    return;
  }

  server.streamFile(file, "text/html");
  file.close();
}

void handlePress() {
  digitalWrite(POWER_PIN, HIGH);
  writeLog("ACTION: Button pressed");
  server.send(200, "text/plain", "OK: Pressed");
}

void handleRelease() {
  digitalWrite(POWER_PIN, LOW);
  writeLog("ACTION: Button released");
  server.send(200, "text/plain", "OK: Released");
}

void handleStatus() {
  server.send(200, "application/json",
    lastStatus ? "{\"status\":\"ON\"}" : "{\"status\":\"OFF\"}"
  );
}

// ================= FIX INI PENTING =================
void handleLog() {

  File file = LittleFS.open("/log.txt", "r");

  if (!file) {
    server.send(404, "text/plain", "NO LOG");
    return;
  }

  server.streamFile(file, "text/plain");
  file.close();
}

// ======================================================
void handleClearLog() {

  LittleFS.remove("/log.txt");

  File f = LittleFS.open("/log.txt", "w");
  f.close();

  webSocket.broadcastTXT("{\"clear\":true}");

  writeLog("LOG CLEARED");

  server.send(200, "text/plain", "CLEARED");
}

// ======================================================
// WEBSOCKET
// ======================================================
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {

  if (type == WStype_CONNECTED) {

    // Menggunakan const char* lebih aman daripada membuat String baru
    const char* statusMsg = lastStatus ? "{\"status\":\"ON\"}" : "{\"status\":\"OFF\"}";
    webSocket.sendTXT(num, statusMsg);

    writeLog("WS: Client " + String(num) + " connected");

    // kirim history log
    sendLogHistory(num);
  }
}

// ======================================================
// WIFI CONNECT
// ======================================================
bool connectWiFi() {

  writeLog("TRY WIFI CONNECT");

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setOutputPower(20.5);
  WiFi.hostname("ESP8266-POWER");

  WiFi.begin(ssid, password);

  int timeout = 0;

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
    timeout++;

    if (timeout > 40) {
      writeLog("WIFI FAIL");
      updateWiFiLED();
      return false;
    }
  }

  writeLog("WIFI OK: " + WiFi.localIP().toString());
  updateWiFiLED();

  return true;
}

// ======================================================
// WIFI MANAGER
// ======================================================
void wifiManager() {

  if (WiFi.status() == WL_CONNECTED) {

    updateWiFiLED();

    retryDelay = 20000;

    if (!wifiWasConnected) {

      writeLog("WIFI RESTORED");

      webSocket.disconnect();
      delay(100);

      webSocket.begin();
      webSocket.onEvent(webSocketEvent);
      webSocket.enableHeartbeat(15000, 3000, 2);

      wifiWasConnected = true;
    }

    return;
  }

  wifiWasConnected = false;

  updateWiFiLED();

  if (millis() - lastReconnectAttempt < retryDelay)
    return;

  lastReconnectAttempt = millis();

  writeLog("WIFI LOST -> RECONNECT");

  WiFi.disconnect();
  WiFi.begin(ssid, password);

  retryDelay = min(retryDelay + 5000, 60000);
}

// ======================================================
// SETUP
// ======================================================
void setup() {

  Serial.begin(115200);
  delay(1000);

  pinMode(POWER_PIN, OUTPUT);
  pinMode(STATUS_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(POWER_PIN, LOW);
  digitalWrite(LED_PIN, HIGH);

  if (!LittleFS.begin()) {
    Serial.println("LFS ERROR");
    ESP.restart();
  }

  writeLog("BOOT START");
  writeLog("RESET: " + ESP.getResetReason());

  // ROUTES
  server.on("/", handleRoot);
  server.on("/log", handleLog);
  server.on("/press", handlePress);
  server.on("/release", handleRelease);
  server.on("/status", handleStatus);
  server.on("/clearlog", handleClearLog);

  server.on("/favicon.ico", []() {
    File file = LittleFS.open("/favicon.ico", "r");
    if (!file) {
      server.send(404, "text/plain", "NO ICON");
      return;
    }
    server.streamFile(file, "image/x-icon");
    file.close();
  });

  server.begin();

  // WS
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  webSocket.enableHeartbeat(15000, 3000, 2);

  connectWiFi();
}

// ======================================================
// LOOP
// ======================================================
void loop() {

  ESP.wdtFeed();

  server.handleClient();
  webSocket.loop();
  wifiManager();

  bool currentStatus = digitalRead(STATUS_PIN) == LOW;

  if (currentStatus != lastStatus &&
      millis() - lastChange > 150) {

    lastChange = millis();
    lastStatus = currentStatus;

    writeLog(lastStatus ? "STATUS ON" : "STATUS OFF");

    // Menggunakan const char* lebih aman daripada membuat String baru
    const char* statusMsg = lastStatus ? "{\"status\":\"ON\"}" : "{\"status\":\"OFF\"}";
    webSocket.broadcastTXT(statusMsg);
  }

  yield();
}
