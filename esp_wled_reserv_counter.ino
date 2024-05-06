#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include "ESPAsyncWebServer.h"
#include <HTTPClient.h>
#include <Arduino_JSON.h>

const char* wifi_network_ssid = "qrf party machine";
const char* wifi_network_password = "ainidergb";

const char* soft_ap_ssid = "testif";
const char* soft_ap_password = "amogus123";

String serverName = "http://4.3.2.1:80/json/state";

AsyncWebServer server(80);

char buffer[1170];

unsigned long lastTime = 0;
unsigned long timerDelayMs = 10000;

void setup() {

  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_APSTA);

  WiFi.softAP(soft_ap_ssid, soft_ap_password);

  WiFi.begin(wifi_network_ssid, wifi_network_password);


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.print("ESP32 IP as soft AP: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("ESP32 IP on the WiFi network: ");
  Serial.println(WiFi.localIP());


  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Hello from AP");
  });

  server.begin();
}

void loop() {

  if ((millis() - lastTime) > timerDelayMs) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      http.begin(serverName.c_str());

      // If you need Node-RED/server authentication, insert user and password below
      //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

      http.addHeader("Content-Type", "application/json");

      snprintf(buffer, 1170, "{\"on\":true,\"bri\":255,\"transition\":7,\"mainseg\":1,\"seg\":[{\"id\":0,\"start\":0,\"stop\":300,\"grp\":1,\"spc\":0,\"of\":0,\"on\":true,\"frz\":false,\"bri\":255,\"cct\":127,\"set\":0,\"n\":\"\",\"col\":[[255,160,0],[0,0,0],[0,0,0]],\"fx\":106,\"sx\":0,\"ix\":255,\"pal\":18,\"c1\":128,\"c2\":128,\"c3\":16,\"sel\":false,\"rev\":false,\"mi\":false,\"o1\":false,\"o2\":false,\"o3\":false,\"si\":0,\"m12\":0},{\"id\":1,\"start\":0,\"stop\":%d,\"grp\":1,\"spc\":0,\"of\":0,\"on\":true,\"frz\":false,\"bri\":255,\"cct\":127,\"set\":0,\"n\":\"Bar\",\"col\":[[100,255,38],[0,0,0],[0,0,0]],\"fx\":0,\"sx\":128,\"ix\":128,\"pal\":0,\"c1\":128,\"c2\":128,\"c3\":16,\"sel\":true,\"rev\":false,\"mi\":false,\"o1\":false,\"o2\":false,\"o3\":false,\"si\":0,\"m12\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0}]}", 170);

      // Send HTTP GET request
      int httpResponseCode = http.POST(String(buffer));

      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }

      // Free resources
      http.end();
    }
    lastTime = millis();
  }
}