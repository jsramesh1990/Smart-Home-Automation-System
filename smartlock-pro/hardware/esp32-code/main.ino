#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <esp_now.h>

// Pin definitions
#define LOCK_PIN 26
#define STATUS_LED 2
#define BUZZER_PIN 25
#define DOOR_SENSOR 34

// Variables
String lockId = "SMARTLOCK_001";
String lockStatus = "locked";
String lastUser = "";
unsigned long lastUnlockTime = 0;
unsigned long autoLockDelay = 30000; // 30 seconds

WebServer server(80);
Preferences preferences;

// Voice recognition states
bool voiceEnabled = true;
String authorizedVoices[5]; // Store voice fingerprints
int voiceCount = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(LOCK_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(DOOR_SENSOR, INPUT);
  
  digitalWrite(LOCK_PIN, HIGH); // Default locked
  
  // Connect to WiFi
  WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(10000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected!");
  
  // Initialize preferences
  preferences.begin("smartlock", false);
  
  // Load saved settings
  autoLockDelay = preferences.getUInt("autolock", 30000);
  
  // Setup server routes
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/lock", HTTP_POST, handleLock);
  server.on("/unlock", HTTP_POST, handleUnlock);
  server.on("/logs", HTTP_GET, handleLogs);
  server.on("/guestcode", HTTP_POST, handleGuestCode);
  server.on("/voice", HTTP_POST, handleVoiceCommand);
  
  server.begin();
  Serial.println("HTTP server started");
  
  // Initialize ESP-NOW for local control
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  }
}

void loop() {
  server.handleClient();
  
  // Auto-lock check
  if (lockStatus == "unlocked" && 
      (millis() - lastUnlockTime) > autoLockDelay) {
    lockDoor("AUTO");
  }
  
  // Door sensor check
  int doorState = digitalRead(DOOR_SENSOR);
  static int lastDoorState = LOW;
  
  if (doorState != lastDoorState) {
    logAccess(doorState == HIGH ? "DOOR_OPENED" : "DOOR_CLOSED", "SYSTEM");
    lastDoorState = doorState;
  }
  
  delay(100);
}

void handleLock() {
  String user = server.arg("user");
  lockDoor(user);
  
  DynamicJsonDocument doc(200);
  doc["status"] = "locked";
  doc["user"] = user;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleUnlock() {
  String user = server.arg("user");
  String code = server.arg("code");
  
  if (validateAccess(user, code)) {
    unlockDoor(user);
    lastUnlockTime = millis();
    
    DynamicJsonDocument doc(200);
    doc["status"] = "unlocked";
    doc["user"] = user;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  } else {
    server.send(403, "application/json", "{\"error\":\"Access denied\"}");
  }
}

void lockDoor(String user) {
  digitalWrite(LOCK_PIN, HIGH);
  lockStatus = "locked";
  digitalWrite(STATUS_LED, LOW);
  
  logAccess("LOCKED", user);
  beep(1);
  
  // Send to server
  sendLogToServer("LOCKED", user);
}

void unlockDoor(String user) {
  digitalWrite(LOCK_PIN, LOW);
  lockStatus = "unlocked";
  digitalWrite(STATUS_LED, HIGH);
  
  logAccess("UNLOCKED", user);
  beep(2);
  
  // Send to server
  sendLogToServer("UNLOCKED", user);
}

void handleVoiceCommand() {
  if (!voiceEnabled) {
    server.send(400, "application/json", "{\"error\":\"Voice disabled\"}");
    return;
  }
  
  String voiceData = server.arg("data");
  String userId = server.arg("userId");
  
  // Simple voice command processing
  if (voiceData.indexOf("unlock") >= 0 || voiceData.indexOf("open") >= 0) {
    if (isVoiceAuthorized(userId)) {
      unlockDoor("VOICE_" + userId);
      server.send(200, "application/json", "{\"status\":\"unlocked\"}");
    } else {
      server.send(403, "application/json", "{\"error\":\"Voice not recognized\"}");
    }
  } else if (voiceData.indexOf("lock") >= 0) {
    lockDoor("VOICE_" + userId);
    server.send(200, "application/json", "{\"status\":\"locked\"}");
  }
}

void logAccess(String action, String user) {
  // Store in EEPROM
  String logEntry = String(millis()) + "," + action + "," + user;
  preferences.putString("log_" + String(millis()), logEntry);
  
  Serial.println("Log: " + logEntry);
}

void sendLogToServer(String action, String user) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://your-server.com/api/logs");
    http.addHeader("Content-Type", "application/json");
    
    DynamicJsonDocument doc(200);
    doc["lockId"] = lockId;
    doc["action"] = action;
    doc["user"] = user;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    
    int httpResponseCode = http.POST(payload);
    http.end();
  }
}

void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}
