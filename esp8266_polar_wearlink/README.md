# Overview

This is designed to link https://www.adafruit.com/product/1077 to http://www.eegsynth.org

# SPIFFS for static files

Please note that you should not only write the firmware to the ESP8266 module, but also the static content for the web interface. The html, css and javascript files located in the data directory should be written to the SPIFS filesystem on the ESP8266. See for example http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html and https://www.instructables.com/id/Using-ESP8266-SPIFFS for instructions.
You will get a "file not found" error if the firmware cannot access the data files.
