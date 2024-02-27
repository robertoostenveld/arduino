# Overview

This is an Arduino sketch for an ESP8266 module connected to an AD8232 ECG sensor module. It uses the ADC to sample the data and writes it as a continuous stream to a computer that is running a FieldTrip buffer.

You can use the webinterface to set the IP address and port of the receiving computer.

## SPIFFS for static files

You should not only write the firmware to the ESP8266 module, but also the static content for the web interface. The html, css and javascript files located in the data directory should be written to the SPIFS filesystem on the ESP8266. See for example http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html and https://www.instructables.com/id/Using-ESP8266-SPIFFS for instructions.

You will get a "file not found" error if the firmware cannot access the data files.

## Arduino ESP8266 filesystem uploader

This Arduino sketch includes a `data` directory with a number of files that should be uploaded to the ESP8266 using the [SPIFFS filesystem uploader](https://github.com/esp8266/arduino-esp8266fs-plugin) tool. At the moment (Feb 2024) the Arduino 2.x IDE does *not* support the SPIFFS filesystem uploader plugin. You have to use the Arduino 1.8.x IDE (recommended), or the command line utilities for uploading the data.
