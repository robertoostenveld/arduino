#ifndef _CONFIG_H_
#define _CONFIG_H_

struct Config {
  int universe;
  int offset;
  int length;
  int leds;
  int white;
  int brightness;
  int hsv;
  int mode;
  int reverse;
  int speed;
  int position;
};

void handleUpdate1(void);
void handleUpdate2(void);
void handleDirList(void);
void handleTemperature(void);
void handleNotFound(void);
void handleRedirect(String);
void handleRedirect(const char *);
void handleStaticFile(String);
void handleStaticFile(const char *);
void handleSettings();

#endif

