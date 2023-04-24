#ifndef DOGSTEP_COMMUNICATION_H
#define DOGSTEP_COMMUNICATION_H

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <AsyncUDP.h>
#include <WiFiUdp.h>
#include <time.h>

const char* ssid = "Pensionato";
const char* password = "492306pp";
const char* id_token = "1fcbaf0d-590b-4440-b4c1-e3c665eafb3e";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3600 * 3;
const int daylightOffset_sec = 0;
char timeStringBuff[50];

const char* serverName = "http://www.google.com";

bool getCurrentTimeStamp() {
  struct tm timeInfo;
  bool getTimer = false;
  for (int i = 0; i < 5; i++) {
    if (getLocalTime(&timeInfo)) {
      getTimer = true;
      break;
    } else {
      Serial.println("Failed to obtain time");
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    }
  }
  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeInfo);
  return getTimer;
}

// JSON data buffer
StaticJsonDocument<250> jsonDocument;
char buffer[250];

// Web server

// global counter
uint32_t step_count = 0;

inline void create_json(uint32_t stepsAmount) {
  // Clear and fill json
  jsonDocument.clear();
  jsonDocument["steps"] = stepsAmount;
  jsonDocument["time"] = timeStringBuff;
  jsonDocument["id"] = id_token;
  serializeJson(jsonDocument, buffer);
}

//////////////// API ENDPOINTS HANDLERS ////////////////
inline void getPedometer() {

  create_json(step_count);
  // server.send(200, "application/json", buffer);
}
//////////////// API ENDPOINTS HANDLERS ////////////////

//////////////// WIFI CONFIGURATION ////////////////

unsigned long lastTime = 0;
unsigned long timerDelay = 10000;
HTTPClient http;
WiFiClient client;

inline bool send_data_to_server() {
  bool ret = false;
  if ((millis() - lastTime) >= timerDelay) {
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED && getCurrentTimeStamp()) {

      // If you need an HTTP request with a content type: application/json, use the following:
      // http.addHeader("Content-Type", "application/json");
      int current_steps = step_count;
      create_json(step_count);
      int httpResponseCode = http.GET(); //(buffer);

      Serial.println("Post pedometer");

      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      if (httpResponseCode == 200) {
        Serial.println("Post successful");
        step_count = step_count - current_steps;
        ret = true;
      } else {
        ret = false;
      }
    } else {
      Serial.println("WiFi Disconnected");
      ret = false;
    }
    lastTime = millis();
  }
  return ret;
}

inline void setup_wifi() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getCurrentTimeStamp();

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
}
//////////////// WIFI CONFIGURATION ////////////////

//////////////// API CONFIGURATION ////////////////

inline void commSetup() {
  setup_wifi();
}

inline bool handleLoop() {
  return send_data_to_server();
}
#endif /* DOGSTEP_COMMUNICATION_H */