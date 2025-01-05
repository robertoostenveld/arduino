#ifndef _WEBINTERFACE_H_
#define _WEBINTERFACE_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <FS.h>

#ifndef ARDUINOJSON_VERSION
#error ArduinoJson version 5 not found, please include ArduinoJson.h in your .ino file
#endif

#if ARDUINOJSON_VERSION_MAJOR != 5
#error ArduinoJson version 5 is required
#endif

/* these are for numbers */
#define N_JSON_TO_CONFIG(x, y)   { if (root.containsKey(y)) { config.x = root[y]; } }
#define N_CONFIG_TO_JSON(x, y)   { root.set(y, config.x); }
#define N_KEYVAL_TO_CONFIG(x, y) { if (server.hasArg(y))    { String str = server.arg(y); config.x = str.toFloat(); } }

/* these are for strings */
#define S_JSON_TO_CONFIG(x, y)   { if (root.containsKey(y)) { strcpy(config.x, root[y]); } }
#define S_CONFIG_TO_JSON(x, y)   { root.set(y, config.x); }
#define S_KEYVAL_TO_CONFIG(x, y) { if (server.hasArg(y))    { String str = server.arg(y); strcpy(config.x, str.c_str()); } }

struct Config {
  char address[32];
  int port;
};

extern Config config;

bool defaultConfig(void);
bool loadConfig(void);
bool saveConfig(void);

void handleUpdate1(void);
void handleUpdate2(void);
void handleDirList(void);
void handleNotFound(void);
void handleRedirect(String);
void handleRedirect(const char *);
bool handleStaticFile(String);
bool handleStaticFile(const char *);
void handleJSON();

#endif
