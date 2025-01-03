# Overview

This is an Arduino sketch for an ExGpill connected to a LOLIN32 Lite development board. It features a webinterface that allows to change the configuration and it streams the EMG/ECG/EEG data as a FieldTrip buffer client to a server elsewhere on the wifi network.

It is compatible with the [EEGsynth](https://www.eegsynth.org).

## SPIFFS for static files

You should not only write the firmware to the ESP32 module, but also the static content for the web interface. The html, css and javascript files located in the data directory should be written to the SPIFS filesystem on the ESP32. See https://github.com/me-no-dev/arduino-esp32fs-plugin.

You will get a "file not found" error if the firmware cannot access the data files.

## Arduino ESP32 filesystem uploader

This Arduino sketch includes a `data` directory with a number of files that should be uploaded to the ESP8266 using the [SPIFFS filesystem uploader](https://github.com/esp8266/arduino-esp8266fs-plugin) tool. At the moment (Feb 2024) the Arduino 2.x IDE does *not* support the SPIFFS filesystem uploader plugin. You have to use the Arduino 1.8.x IDE (recommended), or the command line utilities for uploading the data.
