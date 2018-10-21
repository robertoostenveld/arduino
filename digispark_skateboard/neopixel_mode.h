#ifndef _NEOPIXEL_MODE_H_
#define _NEOPIXEL_MODE_H_

#include <Adafruit_NeoPixel.h>

#define PIN            0
#define NUMPIXELS      60

// in the original version these can be configured dynamically
#define CONFIG_SPEED   1.0
#define CONFIG_SPLIT   1

#define ROUND(x)   (int(x + 0.5))
#define ABS(x)     (x * (x < 0 ? -1 : 1))
#define MIN(x, y)  (x < y ? x : y)
#define MAX(x, y)  (x > y ? x : y)
#define WRAP360(x) (x > 0 ? (x - int((x)/360)*360) : (x - int((x)/360 - 1)*360))  // between    0 and 360
#define WRAP180(x) (WRAP360(x) < 180 ? WRAP360(x) : WRAP360(x) - 360)             // between -180 and 180
#define BALANCE(l, x1, x2)  ((x1) * (1. - l) + (x2) * l)

extern Adafruit_NeoPixel strip;

// the naming of these modes corresponds to my ESP8266 Artnet Neopixel module
void mode1(uint8_t *);
void mode4(uint8_t *);
void mode10(uint8_t *);
void mode12(uint8_t *);

void map_hsv_to_rgb(int *, int *, int *);

#endif
