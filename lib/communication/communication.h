#ifndef DOGSTEP_COMMUNICATION_H
#define DOGSTEP_COMMUNICATION_H

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include <time.h>

const char* ssid = "TP-Link_A885";
const char* password = "70901573";

// JSON data buffer
StaticJsonDocument<250> jsonDocument;
char buffer[250];

// Web server
WebServer server(80);

// global counter
uint32_t step_count = 0;

inline void create_json(uint32_t stepsAmount) {
  // Create and populate date for request
  struct tm current_ts;
  getLocalTime(&current_ts);
  char ts_char[50] = {0};
  strftime(ts_char, sizeof(ts_char), "%Y-%m-%d %H:%M:%S", &current_ts);

  // Clear and fill json
  jsonDocument.clear();
  jsonDocument["steps"] = stepsAmount;
  jsonDocument["requestDate"] = ts_char;
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
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());
}
//////////////// WIFI CONFIGURATION ////////////////

//////////////// API CONFIGURATION ////////////////
// setup API resources
inline void setup_routing() {
  // Debug
  Serial.println("Starting routing");

  // Setup endpoints
  server.on("/pedometer", HTTP_GET, getPedometer);

  // start server
  server.begin();

  Serial.println("Routing started.");
}
//////////////// API CONFIGURATION ////////////////

//////////////// SENSOR CONFIGURATION ////////////////
// TODO: unification
inline void handleClientTask(void* parameter) {
  while (true) {
    // Handle upcoming requests in the API
    server.handleClient();
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

inline void setup_task() {
  xTaskCreate(handleClientTask,   // call method
              "Read sensor data", // task name
              1000,               // size (bytes)
              NULL,               // params
              1,                  // priority
              NULL                // task handler
  );
}
//////////////// SENSOR CONFIGURATION ////////////////

inline void commSetup() {
  setup_wifi();
  setup_routing();
  setup_task();
}
#endif /* DOGSTEP_COMMUNICATION_H */