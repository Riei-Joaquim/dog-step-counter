#ifndef DOGSTEP_COMMUNICATION_H
#define DOGSTEP_COMMUNICATION_H

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include <time.h>

const char* ssid = "Pensionato";
const char* password = "492306pp";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = -3600 * 3;
char timeStringBuff[50];

void getCurrentTimeStamp() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeInfo);
}

// JSON data buffer
StaticJsonDocument<250> jsonDocument;
char buffer[250];

// Web server
WebServer server(80);

// ip static
// Set your Static IP address
IPAddress local_IP(192, 168, 1, 184);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// global counter
uint32_t step_count = 0;

inline void create_json(uint32_t stepsAmount) {
  getCurrentTimeStamp();

  // Clear and fill json
  jsonDocument.clear();
  jsonDocument["steps"] = stepsAmount;
  jsonDocument["requestDate"] = timeStringBuff;
  serializeJson(jsonDocument, buffer);
}

//////////////// API ENDPOINTS HANDLERS ////////////////
inline void getPedometer() {
  Serial.println("Get pedometer called");
  create_json(step_count);
  server.send(200, "application/json", buffer);
}
//////////////// API ENDPOINTS HANDLERS ////////////////

//////////////// WIFI CONFIGURATION ////////////////
inline void setup_wifi() {
  WiFi.begin(ssid, password);

  // Configures static IP address
  // if (!WiFi.config(local_IP, gateway, subnet)) {
  //  Serial.println("STA Failed to configure");
  //}
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getCurrentTimeStamp();
}
//////////////// WIFI CONFIGURATION ////////////////

//////////////// API CONFIGURATION ////////////////
// setup API resources
inline void setup_routing() {
  // Debug
  Serial.println("Starting routing");

  // Setup endpoints
  server.on("/pedometer", HTTP_GET, getPedometer);
  server.enableDelay(false);
  // start server
  server.begin();

  Serial.println("Routing started.");
}
//////////////// API CONFIGURATION ////////////////

inline void commSetup() {
  setup_wifi();
  setup_routing();
}

inline void handleLoop() {
  server.handleClient();
}
#endif /* DOGSTEP_COMMUNICATION_H */