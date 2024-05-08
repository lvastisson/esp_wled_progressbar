# WLED API-based progressbar running on esp32/esp8266

<img src="https://raw.githubusercontent.com/lvastisson/esp_wled_progressbar/master/webui.png" alt="web ui of the app" height="120"/>
<img src="https://raw.githubusercontent.com/lvastisson/esp_wled_progressbar/master/wled_progressbar.gif" alt="demonstration in use using wled app peek function" height="=120"/>

This is a basic esp32/esp8266 program that provides configurable progressbar functionality for WLED software through it's API.
The code is by no means perfect and not tested in it's entirety.

## Features

- storage of current, timer start and timer end times in the EEPROM (state doesn't reset to 0 on cold reboot)
- simple web ui at http://192.168.4.1:80 that allows you to set the progressbar start/end datetimes and also sync the current time from your browser to the esp (the time also gets synced to your wled, however you can disable this behaviour by commenting out line calling `syncWledTime()` function)

## Usage

Works almost out-of-box with wled setup where only 1 segment is used. This program works by creating a 2nd segment itself and then *moves* the progressbar by manipulating the `start` and `end` values of both segments.

Before use you must check and change the following variables found in `esp_wled_progressbar.ino`:
```C++
const char* wifi_network_ssid = "WLED_SSID_HERE";
const char* wifi_network_password = "WLED_PASSWORD_HERE";
String wled_api = "http://4.3.2.1:80/json/state";
String wled_sync = "http://4.3.2.1:80/win?ST=";
const short ledcount = 300;
```

## Development guide

Develop the html wherever you'd like (for example index.html found here) and later before compiling copy the entire file contents to the value of `const char index_html[]`

## Known bugs

- timekeeping functionality overflow after 50 days of continous operation due to the limitations of esp millis() function
- sometimes buggy behaviour when current time is out of bounds of start/end times
