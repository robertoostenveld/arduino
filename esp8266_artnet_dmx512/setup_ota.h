#ifndef _SETUP_OTA_H_
#define _SETUP_OTA_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <FS.h>

#define JSON_TO_CONFIG(x, y)   { if (root.containsKey(y)) { config.x = root[y]; } }
#define CONFIG_TO_JSON(x, y)   { root.set(y, config.x); }
#define KEYVAL_TO_CONFIG(x, y) { if (server.hasArg(y))    { String str = server.arg(y); config.x = str.toInt(); } }

#define S_JSON_TO_CONFIG(x, y)   { if (root.containsKey(y)) { strcpy(config.x, root[y]); } }
#define S_CONFIG_TO_JSON(x, y)   { root.set(y, config.x); }
#define S_KEYVAL_TO_CONFIG(x, y) { if (server.hasArg(y))    { String str = server.arg(y); strcpy(config.x, str.c_str()); } }

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

struct Config {
  int universe;
  int channels;
  int delay;
};

#endif // _SETUP_OTA_H_
