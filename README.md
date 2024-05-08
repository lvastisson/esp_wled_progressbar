# WLED API-based progressbar running on esp32/esp8266

THis is a basic esp32/esp8266 program that provides configurable progressbar functionality for WLED software. It is by no means perfect code and not entirely tested.

## Features

- storage of current, timer start and timer end times in the EEPROM (state doesn't reset to 0 on cold reboot)
- simple web ui at http://192.168.4.1:80 that allows you to set the progressbar start/end datetimes and also sync the current time from your browser to the esp (the time also gets synced to your wled, however you can disable this behaviour by commenting out line calling `syncWledTime()` function)

## Development guide

Develop the html wherever you'd like (for example index.html found here) and later before compiling copy the entire file contents to the value of `const char index_html[]`

## Known bugs

- timekeeping functionality overflow after 50 days of continous operation due to the limitations of esp millis() function
- sometimes buggy behaviour when current time is out of bounds of start/end times
