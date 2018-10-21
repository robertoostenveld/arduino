/*
    This is a subset of the color modes supported by my esp8266_artnet_neopixel sketch, see
    https://github.com/robertoostenveld/arduino/tree/master/esp8266_artnet_neopixel

    The functions have the same name and more or less similar functionality, but have been
    simplified to save memory.
*/

#include "neopixel_mode.h"
#include "colorspace.h"

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB);

#define ENABLE_MODE1
#define ENABLE_MODE4
#define ENABLE_MODE10
#define ENABLE_MODE12

/************************************************************************************/

void mode1(uint8_t * data) {
#ifdef ENABLE_MODE1
  int r, g, b;

  r = data[0];
  g = data[1];
  b = data[2];

  for (int pixel = 0; pixel < strip.numPixels(); pixel++)
    strip.setPixelColor(pixel, r, g, b);
  strip.show();
#endif
}

/************************************************************************************/

void mode4(uint8_t * data) {
#ifdef ENABLE_MODE4
  int r, g, b;

  // determine the current phase in the temporal cycle
  float phase = CONFIG_SPEED * data[6] * millis() * 0.360;
  phase = WRAP360(phase);

  // pick between the two colors
  if (phase < 180) {
    r = data[0];
    g = data[1];
    b = data[2];
  }
  else {
    r = data[3];
    g = data[4];
    b = data[5];
  }

  for (int pixel = 0; pixel < strip.numPixels(); pixel++)
    strip.setPixelColor(pixel, r, g, b);
  strip.show();
#endif
}

/************************************************************************************/

void mode10(uint8_t * data) {
#ifdef ENABLE_MODE10
  int r, g, b;

  // determine the current phase in the temporal cycle
  float phase = CONFIG_SPEED * data[6] * millis() * 0.360;

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    float position = (360. * pixel / (strip.numPixels() - 1)) * CONFIG_SPLIT - phase;
    position = WRAP360(position);

    if (position < 180) {
      r = data[0];
      g = data[1];
      b = data[2];
    }
    else {
      r = data[3];
      g = data[4];
      b = data[5];
    }

    strip.setPixelColor(pixel, r, g, b);
  }
  strip.show();
#endif
};

/************************************************************************************/

void mode12(uint8_t * data) {
#ifdef ENABLE_MODE12

  float saturation = 1. * data[0];
  float value      = 1. * data[1] ;
  float speed      = CONFIG_SPEED * data[2];

  // determine the current phase in the temporal cycle
  float phase = (speed * millis()) * 0.360;

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    float hue = (360. * pixel / strip.numPixels()) * CONFIG_SPLIT - phase;
    hue = WRAP360(hue);

    int r = hue;             // hue, between 0-360
    int g = saturation;      // saturation, between 0-255
    int b = value;           // value, between 0-255
    map_hsv_to_rgb(&r, &g, &b);

    strip.setPixelColor(pixel, r, g, b);
  }
  strip.show();
#endif
};

/************************************************************************************/

void map_hsv_to_rgb(int *r, int *g, int *b) {
  hsv in;
  rgb out;
  in.h = 360. * (*r) / 256.;
  in.s =   1. * (*g) / 255.;
  in.v =   1. * (*b) / 255.;
  out = hsv2rgb(in);
  (*r) = out.r * 255;
  (*g) = out.g * 255;
  (*b) = out.b * 255;
}
