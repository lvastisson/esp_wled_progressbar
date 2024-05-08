#ifdef ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#endif
#include "ESPAsyncWebServer.h"
#include <AsyncElegantOTA.h>
#include <EEPROM.h>

// define the number of bytes you want to access
#define EEPROM_SIZE 12

WiFiClient client;
HTTPClient http;

// #define NEOPIXEL 21

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Reservi Counter UI</title>
  <style>
    body {
      font-family: Arial, Helvetica, sans-serif;

      width: 100vw;
      height: 100vh;
      margin: 0;
      padding: 0;
      display: flex;
      justify-content: center;
      align-items: center;
    }

    button {
      border: none;
      border-radius: 8px;
      padding: 8px 16px;
      background-color: rgb(14, 14, 171);
      color: white;
    }

    button:hover {
      background-color: rgb(43, 43, 113);
      cursor: pointer;
    }

    .container {
      padding: 16px;
      display: flex;
      flex-direction: column;
      gap: 16px;
    }

    .sync {
      display: flex;
      flex-direction: column;
      gap: 8px;
    }

    .log {
      position: absolute;
      bottom: 0;
      left: 0;
      overflow-y: scroll;
      height: 20vh;
      width: 100vw;
      color: rgb(236, 236, 236);
    }

    .timer {
      display: flex;
      flex-direction: column;
      gap: 8px;
    }

    .alert {
      -webkit-animation: blink 1s infinite;
      -moz-animation: blink 1s infinite;
      -o-animation: blink 1s infinite;
      animation: blink 1s infinite;
    }

    .green {
      color: green;
    }

    .red {
      color: red;
    }

    @-webkit-keyframes blink {
      0%,
      49% {
        background-color: #c4d625;
      }

      50%,
      100% {
        background-color: #e50000;
      }
    }
  </style>
</head>

<body>
  <div class="container">
    <div class="timer">
      <div>
        <label for="timerstart">Start:</label>
        <!-- <input type="number" name="timerstart" id="timerstart" value="1714510800"> -->
        <input type="datetime-local" name="timerstart" id="timerstart">
      </div>
      <div>
        <label for="timerend">End:</label>
        <!-- <input type="number" name="timerend" id="timerend" value="1718359200"> -->
        <input type="datetime-local" name="timerend" id="timerend">
      </div>
      <div>
        <button onclick="setReserv()">Update reserv counter start & end time</button>
      </div>
    </div>
    <div class="sync">
      <div id="syncstatus" class="green">time diff (esp32 - local): </div>
      <button id="syncbtn" onclick="syncTime()">Sync time</button>
    </div>
    <div class="log">
      <div>Debug log:</div>
      <div id="log"></div>
    </div>
  </div>
</body>
<script>
  let data;

  const logHtmlEl = document.querySelector('#log');
  const timerStartEl = document.querySelector('input#timerstart');
  const timerEndEl = document.querySelector('input#timerend');
  const syncTextEl = document.querySelector('#syncstatus');
  const syncBtnEl = document.querySelector('#syncbtn');

  // W chatgpt, google literally only gave results using jQuery
  function unixTimeToDateTimeLocal(unixTime) {
    const date = new Date(unixTime * 1000);
    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');
    const hours = String(date.getHours()).padStart(2, '0');
    const minutes = String(date.getMinutes()).padStart(2, '0');
    const dateTimeString = `${year}-${month}-${day}T${hours}:${minutes}`;
    return dateTimeString;
  }

  function dateTimeLocalToUnixTime(dateTime) {
    return Math.round(parseInt(new Date(dateTime).getTime()) / 1000);
  }

  const logToHtml = async (raw) => {
    console.log(raw);
    let elem = document.createElement('div');
    if (raw.text) {
      let text = await raw.text();
      elem.innerText = `[${raw.status}] ${text}`;
    } else {
      elem.innerText = `[raw] ${raw}`;
    }
    logHtmlEl.appendChild(elem);
  }

  const syncTime = () => {
    const utcnow = Math.round(Date.now() / 1000);

    fetch(`/sync?t=${utcnow}`).then(async function (response) {
      logToHtml(response);
      syncBtnEl.className = "";
      syncTextEl.className = "green";
      getData();
    }).catch(function (err) {
      logToHtml('Fetch Error: ' + err);
    });
  }

  const setReserv = () => {
    const timerstart = dateTimeLocalToUnixTime(timerStartEl.value);
    const timerend = dateTimeLocalToUnixTime(timerEndEl.value);

    fetch(`/set?s=${timerstart}&e=${timerend}`).then(async function (response) {
      logToHtml(response);
      getData();
    }).catch(function (err) {
      logToHtml('Fetch Error: ' + err);
    });
  }

  const updateDisplay = () => {
    timerStartEl.value = unixTimeToDateTimeLocal(data.start);
    timerEndEl.value = unixTimeToDateTimeLocal(data.end);

    const syncDiff = parseInt(data.currenttime) - Math.round(Date.now() / 1000);
    if (Math.abs(syncDiff) > 10) {
      syncBtnEl.className = "alert";
      syncTextEl.className = "red";
    }

    syncTextEl.innerText = 'time diff (esp32 - local): ' + syncDiff;
  }

  const getData = () => {
    fetch(`/get`).then(async function (response) {
      let rawResponse = await response.text();
      logToHtml(rawResponse);
      data = await JSON.parse(rawResponse);
      updateDisplay();
    }).catch(function (err) {
      logToHtml('Fetch Error: ' + err);
    });
  }

  getData();
</script>
</html>
)rawliteral";

// EEPROM funcs ref: https://forum.arduino.cc/t/saving-an-unsigned-long-int-to-internal-eeprom/487205
unsigned int EEPROM_readint(int address) 
{
  unsigned int word = word(EEPROM.read(address), EEPROM.read(address+1));
  return word;
}

//write word to EEPROM
void EEPROM_writeint(int address, int value) 
{
  EEPROM.write(address,highByte(value));
  EEPROM.write(address+1 ,lowByte(value));
}

// read double word from EEPROM, give starting address
unsigned long EEPROM_readlong(int address)
{
  //use word read function for reading upper part
  unsigned long dword = EEPROM_readint(address);
  //shift read word up
  dword = dword << 16;
  // read lower word from EEPROM and OR it into double word
  dword = dword | EEPROM_readint(address+2);
  return dword;
}

//write long integer into EEPROM
void EEPROM_writelong(int address, unsigned long value) 
{
  //truncate upper part and write lower part into EEPROM
  EEPROM_writeint(address+2, word(value));
  //shift upper part down
  value = value >> 16;
  //truncate and write
  EEPROM_writeint(address, word(value));
}


const char* wifi_network_ssid = "qrf party machine";
const char* wifi_network_password = "ainidergb";

const char* soft_ap_ssid = "reserv counter ui";
const char* soft_ap_password = "altmaereserv";

String wled_api = "http://4.3.2.1:80/json/state";
String wled_sync = "http://4.3.2.1:80/win?ST=";

AsyncWebServer server(80);

char buffer[412];

unsigned long lastTime = 0;
unsigned long timerDelayMs = 5000;

unsigned long starttime = millis() / 1000;
unsigned long localtimee = starttime;
bool syncrequested = false;

const short ledcount = 300;
short currled = -1;
unsigned long counterstart = 1714510800;
unsigned long counterend = 1718359200;

void time_set(unsigned long time) {
  starttime = millis() / 1000;
  localtimee = time;
}
unsigned long time_get() {
  return (millis() / 1000 - starttime + localtimee);
}
void time_save() {
  EEPROM_writelong(0, time_get());
  EEPROM.commit();
}

void counter_save() {
  EEPROM_writelong(4, counterstart);
  EEPROM_writelong(8, counterend);
  EEPROM.commit();
}
void counter_load() {
  counterstart = EEPROM_readlong(4);
  counterend = EEPROM_readlong(8);
}

double reservini_progress() {
  double curr_timee = time_get();
  unsigned long start_to_end_diff = counterend - counterstart;
  return 100 - (((double)counterend - curr_timee) / (double)start_to_end_diff * (double)100);
}
long led_progress() {
  return constrain((long)((double)(reservini_progress() * (double)ledcount) / (double)100), 0, ledcount - 1);
}

void setup() {
  Serial.begin(115200);
  // wait for serial to initialize
  delay(1000);

  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);

  // REQUIRED ON FIRST FLASH
  // EEPROM_writelong(0, 1715059660);
  // counter_save();
  // EEPROM.commit();

  localtimee = EEPROM_readlong(0);
  Serial.print("Local time loaded from EEPROM: ");
  Serial.println(localtimee);
  time_set(localtimee);

  counter_load();

#ifdef ESP32
  Wifi.mode(WIFI_MODE_APSTA);
#else
  WiFi.mode(WIFI_AP_STA);
#endif
  WiFi.softAP(soft_ap_ssid, soft_ap_password);
  WiFi.begin(wifi_network_ssid, wifi_network_password);

  Serial.println("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    // neopixelWrite(NEOPIXEL, 0, 0, RGB_BRIGHTNESS); // Blue
    delay(250);
    // neopixelWrite(NEOPIXEL, 0, 0, 0); // Off / black
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
      // save the new epoch time to flash
      time_save();
      currled = 0;
      syncrequested = true;
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "HTTP GET /sync request sent to your ESP on input field (" + inputParam + ") with value: " + inputMessage);
  });

  // get counter start/end times & current time endpoint
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"start\":" + String(counterstart) + ",\"end\":" + String(counterend) + ",\"currenttime\":" + String(time_get()) + "}");
  });

  // set counter start & end times endpoint
  server.on("/set", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    String inputMessage2;
    String inputParam2;

    if (request->hasParam("s")) {
      inputMessage = request->getParam("s")->value();
      inputParam ="s";
      counterstart = strtoul(inputMessage.c_str(), NULL, 10);
      currled = 0;
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }

    if (request->hasParam("e")) {
      inputMessage2 = request->getParam("e")->value();
      inputParam2 ="e";
      counterend = strtoul(inputMessage2.c_str(), NULL, 10);
      currled = 0;
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }

    // save new counter start/end values to flash
    counter_save();

    Serial.println(inputMessage);
    request->send(200, "text/plain", inputParam + "=" + inputMessage + ";" + inputParam2 + "=" + inputMessage2);
  });

  AsyncElegantOTA.begin(&server);
  server.begin();

  // neopixelWrite(NEOPIXEL,0,RGB_BRIGHTNESS,0); // Green
}

void syncWledTime() {
  String wled_sync_path = wled_sync + String(time_get());
#ifdef ESP32
  http.begin(wled_sync_path.c_str());
#else
  http.begin(client, wled_sync_path.c_str());
#endif
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

void loop() {

  if ((millis() - lastTime) > timerDelayMs) {
    if (WiFi.status() == WL_CONNECTED) {
      if (syncrequested) {
        syncWledTime();
        delay(3000);
        syncrequested = false;
      }
      short curr_progress = led_progress();

      if (currled != curr_progress) {
        // neopixelWrite(NEOPIXEL,0,RGB_BRIGHTNESS,0); // Green

#ifdef ESP32
        http.begin(wled_api.c_str());
#else
        http.begin(client, wled_api.c_str());
#endif
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

          // save current epoch time to flash
          time_save();

          // store the led length that was last set
          currled = curr_progress;
        } else {
          Serial.print("Error code: ");
          Serial.println(httpResponseCode);
        }

        // Free resources
        http.end();
      }
    } else {
      // neopixelWrite(NEOPIXEL,138,70,170); // Purple
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      delay(500);
      WiFi.reconnect();
    }

    lastTime = millis();
  }
}