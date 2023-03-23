#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include <time.h>

const char* ssid     = "TP-Link_A885";
const char* password = "70901573";

// JSON data buffer
StaticJsonDocument<250> jsonDocument;
char buffer[250];

void create_json(float stepsAmount) {  
  // Create and populate date for request
  struct tm current_ts;
  getLocalTime(&current_ts);
  char ts_char[50] = {0};
  strftime(ts_char, sizeof(ts_char),"%Y-%m-%d %H:%M:%S", &current_ts);

  // Clear and fill json
  jsonDocument.clear();
  jsonDocument["steps"] = stepsAmount;
  jsonDocument["requestDate"] = ts_char;
  serializeJson(jsonDocument, buffer);  
}

// Web server
WebServer server(80);

//////////////// API ENDPOINTS HANDLERS ////////////////
void getPedometer() {
  Serial.println("Get pedometer called");
  create_json(1000);
  server.send(200, "application/json", buffer);
}
//////////////// API ENDPOINTS HANDLERS ////////////////


//////////////// WIFI CONFIGURATION ////////////////
void setup_wifi() {
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
void setup_routing() {
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
void read_sensor_data(void * parameter) {
   for (;;) {
     // delay the task for 500ms
     vTaskDelay(500 / portTICK_PERIOD_MS);
   }
}

void setup_task() {     
  xTaskCreate(     
  read_sensor_data,      // call method
  "Read sensor data",    // task name
  1000,                  // size (bytes)
  NULL,                  // params
  1,                     // priority
  NULL                   // task handler
  );     
}
//////////////// SENSOR CONFIGURATION ////////////////

void setup() {
  setup_wifi();
  setup_routing();
  setup_task();
}

void loop() {
  // Handle upcoming requests in the API
  server.handleClient();
}
