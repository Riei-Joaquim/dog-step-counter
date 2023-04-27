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
const char* id_token = "c8571e53-1af7-4fd1-aca4-bd77941d8a5e";

const char* ntpServer = "pool.ntp.org";
const char* ntpServer1 = "1.br.pool.ntp.org";
const char* ntpServer2 = "2.br.pool.ntp.org";
const long gmtOffset_sec = -3600 * 3;
const int daylightOffset_sec = 0;
char timeStringBuff[50];

// Lets Encrypt Root Certificate (Self Signed)
static const char* root_ca = "-----BEGIN CERTIFICATE-----\n"
                             "MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\n"
                             "RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\n"
                             "VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\n"
                             "DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\n"
                             "ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\n"
                             "VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\n"
                             "mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\n"
                             "IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\n"
                             "mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\n"
                             "XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\n"
                             "dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\n"
                             "jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\n"
                             "BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\n"
                             "DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\n"
                             "9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\n"
                             "jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\n"
                             "Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\n"
                             "ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\n"
                             "R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\n"
                             "-----END CERTIFICATE-----\n";

const char* serverName = "https://petrepet-back.onrender.com/historic";

bool getCurrentTimeStamp() {
  struct tm timeInfo;
  bool getTimer = false;
  for (int i = 0; i < 5; i++) {
    if (getLocalTime(&timeInfo)) {
      getTimer = true;
      break;
    } else {
      Serial.println("Failed to obtain time");
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, ntpServer1, ntpServer2);
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
  jsonDocument["dog_id"] = id_token;
  jsonDocument["time"] = timeStringBuff;

  serializeJson(jsonDocument, buffer);
}

unsigned long lastTime = 0;
unsigned long timerDelay = 10000;
HTTPClient http;
WiFiClient client;

inline bool send_data_to_server() {
  bool ret = false;
  if ((millis() - lastTime) >= timerDelay) {
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED && getCurrentTimeStamp()) {
      http.addHeader("Content-Type", "application/json");
      int current_steps = step_count;
      create_json(current_steps);
      Serial.print("Post pedometer: ");
      Serial.println(current_steps);
      int httpResponseCode = http.POST(buffer);

      Serial.print("HTTP Response code: ");
      Serial.println(http.errorToString(httpResponseCode));
      Serial.println(httpResponseCode);
      Serial.println(buffer);

      if (httpResponseCode == 201) {
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

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, ntpServer1, ntpServer2);
  getCurrentTimeStamp();

  //  Your Domain name with URL path or IP address with path
  http.begin(serverName, root_ca);
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