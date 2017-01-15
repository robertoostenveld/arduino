#ifndef _CONFIG_H_
#define _CONFIG_H_

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
  int universe;
  int offset;
  int pixels;
  int leds;
  int white;
  int brightness;
  int hsv;
  int mode;
  int reverse;
  int speed;
  int position;
};

bool initialConfig(void);
bool loadConfig(void);
bool saveConfig(void);

void handleUpdate1(void);
void handleUpdate2(void);
void handleDirList(void);
void handleNotFound(void);
void handleRedirect(String);
void handleRedirect(const char *);
void handleStaticFile(String);
void handleStaticFile(const char *);
void handleJSON();

#endif

