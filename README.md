# WLED API-based progressbar running on esp32/esp8266

THis is a basic esp32/esp8266 program that provides configurable progressbar functionality for WLED software. It is by no means perfect code and not entirely tested.

## Known bugs

- timekeeping functionality overflow after 50 days of continous operation due to the limitations of esp millis() function
- sometimes buggy behaviour when current time is out of bounds of start/end times

## Features

- storage of current, timer start and timer end times in the EEPROM (state doesn't reset to 0 on cold reboot)
- web ui at http://192.168.4.1:80
