# Overview

This is a demonstration and test sketch for configuring persistent options via the web interface. This strategy is used in a number of my functional ESP32 and ESP8266 sketches. It consists of a `settings.json` file in the SPIFFS filesystem and a `settings.html` file with some javascript.

The settings can be changed in the webbrowser at http://192.168.1.xxx/settings and clicking "Send". This results in `Webserver` parsing the arguments and placing the variables in the header, but also places the query string in the body.

The following results in `Webserver` parsing the arguments and placing the variables in the header.

    curl -X POST 'http://192.168.1.xxx/json?var1=11&var2=22&var3=33'
    curl -X PUT  'http://192.168.1.xxx/json?var1=11&var2=22&var3=33'

The following is not parsed, but only places the JSON data in the body.

    curl -X POST http://192.168.1.xxx/json -d '{"var1":11,"var2":22,"var3":33}'
    curl -X PUT  http://192.168.1.xxx/json -d '{"var1":11,"var2":22,"var3":33}'

## SPIFFS for static files

You should not only write the firmware to the ESP32 module, but also the static content for the web interface. The html, css and javascript files located in the data directory should be written to the SPIFS filesystem on the ESP32. See https://github.com/me-no-dev/arduino-esp32fs-plugin.

You will get a "file not found" error if the firmware cannot access the data files.

## Arduino ESP32 filesystem uploader

This Arduino sketch includes a `data` directory with a number of files that should be uploaded to the ESP32 using the [SPIFFS filesystem uploader](https://github.com/me-no-dev/arduino-esp32fs-plugin) tool. At the moment (Feb 2024) the Arduino 2.x IDE does *not* support the SPIFFS filesystem uploader plugin. You have to use the Arduino 1.8.x IDE (recommended), or the command line utilities for uploading the data.
