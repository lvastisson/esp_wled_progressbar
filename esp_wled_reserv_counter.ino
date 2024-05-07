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
  <div id="abc"></div>
</body>
<script>
  const syncTime = () => {
    const utcnow = Math.round(Date.now() / 1000);

    fetch(`/sync?t=${utcnow}`).then(function(response) {
      document.querySelector('#abc').textContent = JSON.stringify(response);
    }).catch(function(err) {
      console.log('Fetch Error: ', err);
    });
  }

  const setReserv = () => {
    const reserv = parseInt(document.querySelector('input#reserv').value);

    fetch(`/set?t=${reserv}`).then(function(response) {
      document.querySelector('#abc').textContent = JSON.stringify(response);
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

String wled_api = "http://4.3.2.1:80/json/state";
String wled_sync = "http://4.3.2.1:80/win&ST=";

AsyncWebServer server(80);

char buffer[412];

unsigned long lastTime = 0;
unsigned long timerDelayMs = 3000;

unsigned long starttime = millis() / 1000;
unsigned long localtimee = starttime;

const short ledcount = 300;
short currled = -1;
unsigned long counterstart = 1715059660;
unsigned long counterend = 1715074200;
unsigned long start_to_end_diff = counterend - counterstart;

void time_set(unsigned long time) {
  starttime = millis() / 1000;
  localtimee = time;
}
unsigned long time_get() {
  return (millis() / 1000 - starttime + localtimee);
}

double reservini_progress() {
  double curr_timee = time_get();
  return 100 - (((double)counterend - curr_timee) / (double)start_to_end_diff * (double)100);
}
long led_progress() {
  return (long)((double)(reservini_progress() * (double)ledcount) / (double)100);
}

void setup() {
  Serial.begin(115200);

  time_set(1715059660);

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
  server.on("/sync", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;

    if (request->hasParam("t")) {
      inputMessage = request->getParam("t")->value();
      inputParam ="t";
      time_set(strtoul(inputMessage.c_str(), NULL, 10));

      HTTPClient http;
      String wled_sync_path = wled_sync + time_get();
      http.begin(wled_sync_path.c_str());
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        Serial.print("(WLED sync) HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      } else {
        Serial.print("(WLED sync) Error code: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "HTTP GET /sync request sent to your ESP on input field (" + inputParam + ") with value: " + inputMessage);
  });

  // reserv deadline set endpoint
  server.on("/set", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;

    if (request->hasParam("t")) {
      inputMessage = request->getParam("t")->value();
      inputParam ="t";
      counterend = strtoul(inputMessage.c_str(), NULL, 10);
      currled = 0;
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "HTTP GET /set request sent to your ESP on input field (" + inputParam + ") with value: " + inputMessage);
  });

  server.begin();

  neopixelWrite(NEOPIXEL,0,RGB_BRIGHTNESS,0); // Green
}

void loop() {

  if ((millis() - lastTime) > timerDelayMs) {
    if (WiFi.status() == WL_CONNECTED) {
      short curr_progress = led_progress();

      if (currled != curr_progress) {
        neopixelWrite(NEOPIXEL,0,RGB_BRIGHTNESS,0); // Green
        HTTPClient http;

        http.begin(wled_api.c_str());
        http.addHeader("Content-Type", "application/json");


        Serial.print("Set progress to n-th led: ");
        Serial.println(curr_progress);

        // compile string template
        snprintf(buffer, 412, "{\"mainseg\":0,\"seg\":[{\"id\":0,\"start\":%d,\"stop\":300},{\"id\":1,\"start\":0,\"stop\":%d,\"grp\":1,\"spc\":0,\"of\":0,\"on\":true,\"frz\":false,\"bri\":194,\"cct\":127,\"set\":0,\"n\":\"\",\"col\":[[0,255,64],[69,255,13],[179,255,0]],\"fx\":67,\"sx\":15,\"ix\":255,\"pal\":4,\"c1\":128,\"c2\":128,\"c3\":16,\"rev\":true,\"mi\":false,\"o1\":false,\"o2\":false,\"o3\":false,\"si\":0,\"m12\":0}]}", curr_progress, curr_progress);

        // Send HTTP POST request
        int httpResponseCode = http.POST(String(buffer));

        if (httpResponseCode > 0) {
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
          String payload = http.getString();
          Serial.println(payload);
          currled = curr_progress;
        } else {
          Serial.print("Error code: ");
          Serial.println(httpResponseCode);
        }

        // Free resources
        http.end();
      }
    } else {
      neopixelWrite(NEOPIXEL,138,70,170); // Purple
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      delay(500);
      WiFi.reconnect();
    }

    lastTime = millis();
  }
}