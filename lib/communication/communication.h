#ifndef DOGSTEP_COMMUNICATION_H
#define DOGSTEP_COMMUNICATION_H

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <AsyncUDP.h>
#include <WiFiUdp.h>
#include <time.h>

const char* ssid = "Pensionato";
const char* password = "492306pp";
const char* id_token = "199";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3600 * 3;
const int daylightOffset_sec = 0;
char timeStringBuff[50];

void getCurrentTimeStamp() {
  struct tm timeInfo;
  for (int i = 0; i < 5; i++) {
    if (getLocalTime(&timeInfo)) {
      break;
    } else {
      Serial.println("Failed to obtain time");
    }
  }
  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeInfo);
}

// JSON data buffer
StaticJsonDocument<250> jsonDocument;
char buffer[250];

// Web server
WebServer server(80);

// global counter
uint32_t step_count = 0;

inline void create_json(uint32_t stepsAmount) {
  step_count = 0;
  getCurrentTimeStamp();

  // Clear and fill json
  jsonDocument.clear();
  jsonDocument["steps"] = stepsAmount;
  jsonDocument["time"] = timeStringBuff;
  jsonDocument["id"] = id_token;
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

// create UDP instance
AsyncUDP udpBroadcast;

// here is broadcast address
const char* udpAddress = "255.255.255.255";
const int receiveUdpPort = 10211;
const int sendUdpPort = 2255;
bool serverDiscovered = false;
String expectedMessageFromServer = "hello collar from server 101";
uint8_t receiveBuffer[30] = "";

void echo_to_server() {

  while (true) {
    Serial.println("wait for server ...");
    // send hello world to server
    udpBroadcast.broadcastTo("hello server|199", sendUdpPort);
    delay(500);

    if (serverDiscovered) {
      break;
    }
  }
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

  // echo to search server
  // This initializes udp and transfer buffer
  for (int i = 0; i < 5; i++) {
    if (udpBroadcast.listen(receiveUdpPort)) {
      Serial.println("listening... ");
      udpBroadcast.onPacket([](AsyncUDPPacket packet) {
        String data(reinterpret_cast<char*>(packet.data()));
        serverDiscovered = (data == expectedMessageFromServer);
      });
      break;
    }
  }

  echo_to_server();
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