#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// WiFi credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Server endpoint
const char* serverUrl = "https://buddyninja-esp32-gps.onrender.com";

// Device ID
const char* deviceId = "ESP32_001";

// Time configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

// Timing variables
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 3600000; // 1 hour in milliseconds
const unsigned long totalDuration = 43200000; // 12 hours in milliseconds
unsigned long startTime = 0;

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Initialize time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Waiting for time synchronization...");
  delay(2000);
  
  startTime = millis();
  
  // Send first payload immediately
  sendPayload();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check if 12 hours have passed
  if (currentTime - startTime >= totalDuration) {
    Serial.println("12-hour duration completed. Stopping transmissions.");
    delay(60000); // Wait 1 minute before checking again
    return;
  }
  
  // Check if it's time to send data (every 1 hour)
  if (currentTime - lastSendTime >= sendInterval) {
    sendPayload();
    lastSendTime = currentTime;
  }
  
  delay(1000); // Check every second
}

String generateRandomHexPayload() {
  String payload = "";
  
  // Generate 4 hex digits for longitude (0000-FFFF)
  for (int i = 0; i < 4; i++) {
    payload += String(random(0, 16), HEX);
  }
  
  // Generate 4 hex digits for latitude (0000-FFFF)
  for (int i = 0; i < 4; i++) {
    payload += String(random(0, 16), HEX);
  }
  
  // Generate 2 hex digits for battery (00-64 hex = 0-100 decimal)
  int battery = random(0, 101); // 0-100%
  String batteryHex = String(battery, HEX);
  if (batteryHex.length() == 1) batteryHex = "0" + batteryHex;
  payload += batteryHex;
  
  payload.toUpperCase();
  return payload;
}

void sendPayload() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Get current time
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return;
    }
    
    char dateStr[11];
    char timeStr[9];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    
    // Generate random hex payload
    String hexPayload = generateRandomHexPayload();
    
    // Create JSON payload
    StaticJsonDocument<256> doc;
    doc["id"] = deviceId;
    doc["payload"] = hexPayload;
    doc["date"] = dateStr;
    doc["time"] = timeStr;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    Serial.println("Sending payload:");
    Serial.println(jsonString);
    
    // Send HTTP POST request
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    int httpResponseCode = http.POST(jsonString);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.print("Response: ");
      Serial.println(response);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}