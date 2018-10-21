/*
    This is a subset and simplified version of the color modes supported by my esp8266_artnet_neopixel sketch, see
    https://github.com/robertoostenveld/arduino/tree/master/esp8266_artnet_neopixel
*/

#include "neopixel_mode.h"
#include "colorspace.h"

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB);
long tic_frame;
float prev;

/*
  mode 1: single uniform color
  channel 1 = red
  channel 2 = green
  channel 3 = blue
  channel 4 = intensity
*/

void mode1(uint8_t * data) {
  int i = 0, r, g, b;
  float intensity;
  
  r         = data[i++];
  g         = data[i++];
  b         = data[i++];
  intensity = 1. * data[i++] / 255.;

  if (CONFIG_HSV)
    map_hsv_to_rgb(&r, &g, &b);

  // scale with the intensity
  r = intensity * r;
  g = intensity * g;
  b = intensity * b;

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    strip.setPixelColor(pixel, r, g, b);
  }
  strip.show();
}

/*
  mode 4: uniform color, blinking between color 1 and color 2
  channel 1  = color 1 red
  channel 2  = color 1 green
  channel 3  = color 1 blue
  channel 4  = color 2 red
  channel 5  = color 2 green
  channel 6  = color 2 blue
  channel 7  = intensity
  channel 8  = speed
  channel 9  = ramp
  channel 10 = duty cycle
*/

void mode4(uint8_t * data) {
  int i = 0, r, g, b, r2, g2, b2;
  float intensity, speed, ramp, duty, phase, balance;
  
  r         = data[i++];
  g         = data[i++];
  b         = data[i++];
  r2        = data[i++];
  g2        = data[i++];
  b2        = data[i++];
  intensity = 1. * data[i++] / 255.;
  speed     = 1. * data[i++] / CONFIG_SPEED;
  ramp      = 1. * data[i++] * 360. / 255.;
  duty      = 1. * data[i++] * 360. / 255.;

  if (CONFIG_HSV) {
    map_hsv_to_rgb(&r, &g, &b);
    map_hsv_to_rgb(&r2, &g2, &b2);
  }

  // the ramp cannot be too wide
  if (duty < 180)
    ramp = (ramp < duty ? ramp : duty);
  else
    ramp = (ramp < (360 - duty) ? ramp : (360 - duty));

  // determine the current phase in the temporal cycle
  phase = (speed * millis()) * 360. / 1000.;

  // prevent rolling back
  if (WRAP180(phase - prev) < 0)
    phase = prev;
  else
    prev = phase;

  phase = WRAP180(phase);
  phase = ABS(phase);

  if (phase <= (duty / 2 - ramp / 4))
    balance = 1;
  else if (phase >= (duty / 2 + ramp / 4))
    balance = 0;
  else if (ramp > 0)
    balance = ((duty / 2 + ramp / 4) - phase) / ( ramp / 2 );

  // apply the balance between the two colors
  r = BALANCE(balance, r, r2);
  g = BALANCE(balance, g, g2);
  b = BALANCE(balance, b, b2);

  // scale with the intensity
  r = intensity * r;
  g = intensity * g;
  b = intensity * b;

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    strip.setPixelColor(pixel, r, g, b);
  }
  strip.show();
}

/*
  mode 10: spinning color wheel with color background
  channel 1  = color 1 red
  channel 2  = color 1 green
  channel 3  = color 1 blue
  channel 4  = color 2 red
  channel 5  = color 2 green
  channel 6  = color 2 blue
  channel 7  = intensity
  channel 8  = speed
  channel 9  = width
  channel 10 = ramp
*/

void mode10(uint8_t * data) {
  int i = 0, r, g, b, r2, g2, b2;
  float intensity, speed, width, ramp, phase;

  r         = data[i++];
  g         = data[i++];
  b         = data[i++];
  r2        = data[i++];
  g2        = data[i++];
  b2        = data[i++];
  intensity = 1. * data[i++] / 255.;
  speed     = 1. * data[i++] / CONFIG_SPEED;
  width     = 1. * data[i++] * 360. / 255.;
  ramp      = 1. * data[i++] * 360. / 255.;

  if (CONFIG_HSV) {
    map_hsv_to_rgb(&r, &g, &b);
    map_hsv_to_rgb(&r2, &g2, &b2);
  }

  // the ramp cannot be too wide
  if (width < 180)
    ramp = (ramp < width ? ramp : width);
  else
    ramp = (ramp < (360 - width) ? ramp : (360 - width));

  // determine the current phase in the temporal cycle
  phase = (speed * millis()) * 360. / 1000.;

  // prevent rolling back
  if (WRAP180(phase - prev) < 0)
    phase = prev;
  else
    prev = phase;

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    int flip = (CONFIG_REVERSE ? -1 : 1);
    float position, balance;

    position = WRAP180((360. * flip * pixel / (strip.numPixels() - 1)) * CONFIG_SPLIT - phase);
    position = ABS(position);

    if (width == 0)
      balance = 0;
    else if (position < (width / 2. - ramp / 2.))
      balance = 1;
    else if (position > (width / 2. + ramp / 2.))
      balance = 0;
    else if (position > 0)
      balance = ((width / 2. + ramp / 2.) - position) / ramp;

    strip.setPixelColor(pixel, intensity * BALANCE(balance, r, r2), intensity * BALANCE(balance, g, g2), intensity * BALANCE(balance, b, b2));
  }
  strip.show();
};

/*
  mode 12: rainbow spinner
  channel 1 = saturation
  channel 2 = value
  channel 3 = speed
*/

void mode12(uint8_t * data) {
  int i = 0;
  float saturation, value, speed, phase;

  saturation = 1. * data[i++];
  value      = 1. * data[i++] ;
  speed      = 1. * data[i++] / CONFIG_SPEED;

  // determine the current phase in the temporal cycle
  phase = (speed * millis()) * 360. / 1000.;

  // prevent rolling back
  if (WRAP180(phase - prev) < 0)
    phase = prev;
  else
    prev = phase;

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    int flip = (CONFIG_REVERSE ? -1 : 1);
    float hue = WRAP360((360. * flip * pixel / strip.numPixels()) * CONFIG_SPLIT - phase);

    int r, g, b;
    r = hue;             // hue, between 0-360
    g = saturation;      // saturation, between 0-255
    b = value;           // value, between 0-255
    map_hsv_to_rgb(&r, &g, &b);

    strip.setPixelColor(pixel, r, g, b);
  }
  strip.show();
};


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
