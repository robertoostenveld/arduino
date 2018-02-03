#ifndef _SETUP_OTA_H_
#define _SETUP_OTA_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <String.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <FS.h>

#define JSON_TO_CONFIG(x, y)   { if (root.containsKey(y)) { config.x = root[y]; } }
#define CONFIG_TO_JSON(x, y)   { root[y] = config.x; }
#define KEYVAL_TO_CONFIG(x, y) { if (server.hasArg(y))    { String str = server.arg(y); config.x = str.toInt(); } }

struct Config {
  int sensors;
  int decimate;
};

void handleNotFound();
void handleStaticFile(String filename);
void handleStaticFile(const char * filename);
void handleJSON();

#endif
