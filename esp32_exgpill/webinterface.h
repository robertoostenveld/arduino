#ifndef _WEBINTERFACE_H_
#define _WEBINTERFACE_H_

struct Config {
  char address[32];
  int port;
};

extern Config config;

bool defaultConfig(void);
bool loadConfig(void);
bool saveConfig(void);
void printConfig(void);
void printRequest(void);

void handleNotFound(void);
void handleRedirect(String);
void handleRedirect(const char *);
bool handleStaticFile(String);
bool handleStaticFile(const char *);
void handleJSON();

#endif
