#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include "ESPAsyncWebServer.h"
#include <HTTPClient.h>
#include <Arduino_JSON.h>

#define NEOPIXEL 21

const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Document</title>
</head>
<body>
  <div>
    <input type="number" name="reserv" id="reserv">
    <button onclick="setReserv()">Update reserv time</button>
  </div>
  <div>
    <button onclick="syncTime()">Sync time</button>
  </div>
</body>
<script>
  const syncTime = () => {
    const utcnow = Math.round(Date.now() / 1000);

    fetch(`/sync&t=${utcnow}`).then(function(response) {
      return response.json();
    }).then(function(data) {
      console.log(data);
    }).catch(function(err) {
      console.log('Fetch Error: ', err);
    });
  }

  const setReserv = () => {
    const reserv = parseInt(document.querySelector('input#reserv').value);

    fetch(`/set&t=${reserv}`).then(function(response) {
      return response.json();
    }).then(function(data) {
      console.log(data);
    }).catch(function(err) {
      console.log('Fetch Error: ', err);
    });
  }
</script>
</html>
)rawliteral";

const char* wifi_network_ssid = "qrf party machine";
const char* wifi_network_password = "ainidergb";

const char* soft_ap_ssid = "testif";
const char* soft_ap_password = "amogus123";

String serverName = "http://4.3.2.1:80/json/state";

AsyncWebServer server(80);

char buffer[1170];

unsigned long lastTime = 0;
unsigned long timerDelayMs = 3000;

unsigned long starttime = millis() / 1000;
unsigned long localtimee = starttime;

const short ledcount = 300;
unsigned long counterstart = 1714998503;
unsigned long reserv = 1714996200;
unsigned long start_to_reserv_diff = reserv - counterstart;

void time_set(unsigned long time) {
  starttime = millis() / 1000;
  localtimee = time;
}
unsigned long time_get() {
  return (millis() / 1000 - starttime + localtimee);
}
double reservini_progress() {
  double curr_timee = time_get();
  // Serial.println(curr_timee);
  // Serial.println(start_to_reserv_diff);
  // Serial.println((double)reserv - curr_timee);
  // Serial.println((double)start_to_reserv_diff * (double)100);
  // Serial.println((((double)reserv - curr_timee) / (double)start_to_reserv_diff * (double)100));
  return 100 - (((double)reserv - curr_timee) / (double)start_to_reserv_diff * (double)100);
}
long led_progress() {
  return (long)((double)(reservini_progress() * (double)ledcount) / (double)100);
}

void setup() {
  Serial.begin(115200);

  time_set(1714998630);

  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.softAP(soft_ap_ssid, soft_ap_password);
  WiFi.begin(wifi_network_ssid, wifi_network_password);


  Serial.println("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    neopixelWrite(NEOPIXEL,0,0,RGB_BRIGHTNESS); // Blue
    delay(250);
    neopixelWrite(NEOPIXEL,0,0,0); // Off / black
    delay(250);
    Serial.print(".");
  }

  Serial.print("ESP32 IP as soft AP: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("ESP32 IP on the WiFi network: ");
  Serial.println(WiFi.localIP());


  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", index_html);
  });

  // utc epoch time set endpoint
  server.on("/set", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;

    if (request->hasParam("t")) {
      inputMessage = request->getParam("t")->value();
      inputParam ="t";
      time_set(strtoul(inputMessage.c_str(), NULL, 10));
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "HTTP GET request sent to your ESP on input field (" + inputParam + ") with value: " + inputMessage);
  });

  // reserv deadline set endpoint
  server.on("/sync", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;

    if (request->hasParam("t")) {
      inputMessage = request->getParam("t")->value();
      inputParam ="t";
      reserv = strtoul(inputMessage.c_str(), NULL, 10);
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "HTTP GET request sent to your ESP on input field (" + inputParam + ") with value: " + inputMessage);
  });

  server.begin();

  neopixelWrite(NEOPIXEL,0,RGB_BRIGHTNESS,0); // Green
}

void loop() {

  if ((millis() - lastTime) > timerDelayMs) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      http.begin(serverName.c_str());
      http.addHeader("Content-Type", "application/json");


      short curr_progress = led_progress();
      Serial.print("Set progress to n-th led: ");
      Serial.println(curr_progress);
      // compile string template
      snprintf(buffer, 1170, "{\"on\":true,\"bri\":255,\"transition\":7,\"mainseg\":1,\"seg\":[{\"id\":0,\"start\":0,\"stop\":300,\"grp\":1,\"spc\":0,\"of\":0,\"on\":true,\"frz\":false,\"bri\":255,\"cct\":127,\"set\":0,\"n\":\"\",\"col\":[[255,160,0],[0,0,0],[0,0,0]],\"fx\":106,\"sx\":0,\"ix\":255,\"pal\":18,\"c1\":128,\"c2\":128,\"c3\":16,\"sel\":false,\"rev\":false,\"mi\":false,\"o1\":false,\"o2\":false,\"o3\":false,\"si\":0,\"m12\":0},{\"id\":1,\"start\":0,\"stop\":%d,\"grp\":1,\"spc\":0,\"of\":0,\"on\":true,\"frz\":false,\"bri\":255,\"cct\":127,\"set\":0,\"n\":\"Bar\",\"col\":[[100,255,38],[0,0,0],[0,0,0]],\"fx\":0,\"sx\":128,\"ix\":128,\"pal\":0,\"c1\":128,\"c2\":128,\"c3\":16,\"sel\":true,\"rev\":false,\"mi\":false,\"o1\":false,\"o2\":false,\"o3\":false,\"si\":0,\"m12\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0},{\"stop\":0}]}", curr_progress);

      // Send HTTP POST request
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