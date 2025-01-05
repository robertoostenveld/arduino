#ifndef _WEBINTERFACE_H_
#define _WEBINTERFACE_H_

#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>  // https://arduinojson.org
#include <FS.h>
#include <SPIFFS.h>

#ifndef ARDUINOJSON_VERSION
#error ArduinoJson version 7 not found, please include ArduinoJson.h in your .ino file
#endif

#if ARDUINOJSON_VERSION_MAJOR != 7
#error ArduinoJson version 7 is required
#endif

/* these are for numbers */
#define N_JSON_TO_CONFIG(x, y)   { if (root.containsKey(y)) { config.x = root[y]; } }
#define N_CONFIG_TO_JSON(x, y)   { root[y] = config.x; }
#define N_KEYVAL_TO_CONFIG(x, y) { if (server.hasArg(y))    { String str = server.arg(y); config.x = str.toFloat(); } }

/* these are for strings */
#define S_JSON_TO_CONFIG(x, y)   { if (root.containsKey(y)) { strcpy(config.x, root[y]); } }
#define S_CONFIG_TO_JSON(x, y)   { root.set(y, config.x); }
#define S_KEYVAL_TO_CONFIG(x, y) { if (server.hasArg(y))    { String str = server.arg(y); strcpy(config.x, str.c_str()); } }

struct Config {
  int var1;
  int var2;
  int var3;
};

extern Config config;

bool defaultConfig(void);
bool loadConfig(void);
bool saveConfig(void);
void printConfig(void);

void handleNotFound(void);
void handleRedirect(String);
void handleRedirect(const char *);
bool handleStaticFile(String);
bool handleStaticFile(const char *);
void handleJSON();

#endif // _WEBINTERFACE_H_
