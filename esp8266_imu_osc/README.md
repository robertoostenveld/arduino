# Overview

This is an Arduino sketch for an ESP8266 module that is connected to a number of inertial motion units (IMUs). It sends the data from the IMUs over OSC to a computer.

See https://robertoostenveld.nl/motion-capture-system/ for more details.

## SPIFFS for static files

You should not only write the firmware to the ESP8266 module, but also the static content for the web interface. The html, css and javascript files located in the data directory should be written to the SPIFS filesystem on the ESP8266. See for example http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html and https://www.instructables.com/id/Using-ESP8266-SPIFFS for instructions.

You will get a "file not found" error if the firmware cannot access the data files.

## SPIFFS for static files

You should not only write the firmware to the ESP8266 module, but also the static content for the web interface. The html, css and javascript files located in the data directory should be written to the SPIFS filesystem on the ESP8266. See for example http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html and https://www.instructables.com/id/Using-ESP8266-SPIFFS for instructions.

You will get a "file not found" error if the firmware cannot access the data files.
