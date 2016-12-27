#include "config.h"
#include "mode.h"

extern Config config;
extern Adafruit_NeoPixel strip;

long tic_frame;

int gamma_l[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

#define RGB  (config.leds==3)
#define RGBW (config.leds==4)

/*
  mode 0: individual pixel control
  channel 1 = pixel 1 red
  channel 2 = pixel 1 green
  channel 3 = pixel 1 blue
  channel 4 = pixel 1 white
  channel 5 = pixel 2 red
  etc.
*/

void mode0(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  for (int i = 0; i < length / config.leds; i++) {
    int pixel   = i + (universe - config.universe) * 512;
    int channel = i * config.leds + config.offset;

    if (pixel >= 0 && pixel < strip.numPixels())
      if (channel >= 0 && (channel + config.leds) < length) {
        if (RGB)
          strip.setPixelColor(pixel, data[channel + 0], data[channel + 1], data[channel + 2]);
        else if (RGBW)
          strip.setPixelColor(pixel, data[channel + 0], data[channel + 1], data[channel + 2], data[channel + 3]);
      }
  }
  strip.show();
} // mode0


/*
  mode 1: single uniform color
  channel 1 = red
  channel 2 = green
  channel 3 = blue
  channel 4 = white
  channel 5 = intensity (this allows scaling a preset RGBW color with a single channel)
*/

void mode1(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  int i = 0, r, g, b, w;
  float intensity;
  if (universe != config.universe)
    return;
  if (RGB && (length - config.offset) < 3 + 1)
    return;
  if (RGBW && (length - config.offset) < 4 + 1)
    return;
  r         = data[config.offset + i++];
  g         = data[config.offset + i++];
  b         = data[config.offset + i++];
  if (RGBW)
    w       = data[config.offset + i++];
  intensity = data[config.offset + i++] / 255.; // fraction between 0 and 1

  // scale with the intensity
  r = intensity * r;
  g = intensity * g;
  b = intensity * b;
  w = intensity * w;

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    if (RGB)
      strip.setPixelColor(pixel, r, g, b);
    else if (RGBW)
      strip.setPixelColor(pixel, r, g, b, w);
  }
  strip.show();
}

/*
  mode 2: two color mixing
  channel 1  = color 1 red
  channel 2  = color 1 green
  channel 3  = color 1 blue
  channel 4  = color 1 white
  channel 5  = color 2 red
  channel 6  = color 2 green
  channel 7  = color 2 blue
  channel 8  = color 2 white
  channel 9  = intensity (this also allows to black out the colors)
  channel 10 = balance (between color 1 and color2)
*/

void mode2(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  int i = 0, r, g, b, w, r2, g2, b2, w2;
  float balance, intensity;
  if (universe != config.universe)
    return;
  if (RGB && (length - config.offset) < 2 * 3 + 2)
    return;
  if (RGBW && (length - config.offset) < 2 * 4 + 2)
    return;
  r         = data[config.offset + i++];
  g         = data[config.offset + i++];
  b         = data[config.offset + i++];
  if (RGBW)
    w       = data[config.offset + i++];
  r2        = data[config.offset + i++];
  g2        = data[config.offset + i++];
  b2        = data[config.offset + i++];
  if (RGBW)
    w2      = data[config.offset + i++];
  intensity = data[config.offset + i++] / 255.; // fraction between 0 and 1
  balance   = data[config.offset + i++] / 255.; // fraction between 0 and 1

  // apply the balance between the two colors
  r = BALANCE(balance, r, r2);
  g = BALANCE(balance, g, g2);
  b = BALANCE(balance, b, b2);
  w = BALANCE(balance, w, w2);

  // scale with the intensity
  r = intensity * r;
  g = intensity * g;
  b = intensity * b;
  w = intensity * w;

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    if (RGB)
      strip.setPixelColor(pixel, r, g, b);
    else if (RGBW)
      strip.setPixelColor(pixel, r, g, b, w);
  }
  strip.show();
}

/*
  mode 3: single uniform color, blinking between the color and black
  channel 1 = red
  channel 2 = green
  channel 3 = blue
  channel 4 = white
  channel 5 = intensity
  channel 6 = speed (number of flashes per unit of time)
  channel 7 = ramp (whether there is a abrubt or more smooth transition)
  channel 8 = duty cycle (the time ratio between the color and black)
*/

void mode3(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  int i = 0, r, g, b, w;
  float intensity, speed, ramp, duty, phase, balance;
  if (universe != config.universe)
    return;
  if (RGB && (length - config.offset) < 3 + 4)
    return;
  if (RGBW && (length - config.offset) < 4 + 4)
    return;
  r         = data[config.offset + i++];
  g         = data[config.offset + i++];
  b         = data[config.offset + i++];
  if (RGBW)
    w       = data[config.offset + i++];
  intensity = data[config.offset + i++] / 255.; // fraction between 0 and 1
  speed     = data[config.offset + i++] / 8.;   // the value 256 maps onto 32 flashes per second
  ramp      = data[config.offset + i++] * (360. / 255.);
  duty      = data[config.offset + i++] * (360. / 255.);

  // the ramp cannot be too wide
  if (duty < 180)
    ramp = (ramp < duty ? ramp : duty);
  else
    ramp = (ramp < (360 - duty) ? ramp : (360 - duty));

  // determine the present phase in the cycle
  phase = (speed * millis()) * 360. / 1000.;
  phase = WRAP180(phase);
  phase = ABS(phase);

  if (phase <= (duty / 2 - ramp / 4))
    balance = 1;
  else if (phase >= (duty / 2 + ramp / 4))
    balance = 0;
  else if (ramp > 0)
    balance = ((duty / 2 + ramp / 4) - phase) / ( ramp / 2 );

  // scale with the intensity
  r = intensity * r;
  g = intensity * g;
  b = intensity * b;
  w = intensity * w;

  // scale with the balance
  r = balance * r;
  g = balance * g;
  b = balance * b;
  w = balance * w;

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    if (RGB)
      strip.setPixelColor(pixel, r, g, b);
    else if (RGBW)
      strip.setPixelColor(pixel, r, g, b, w);
  }
  strip.show();
}

/*
  mode 4: uniform color, blinking between color 1 and color 2
  channel 1  = color 1 red
  channel 2  = color 1 green
  channel 3  = color 1 blue
  channel 4  = color 1 white
  channel 5  = color 2 red
  channel 6  = color 2 green
  channel 7  = color 2 blue
  channel 8  = color 2 white
  channel 9  = intensity
  channel 10 = speed
  channel 11 = ramp
  channel 12 = duty cycle
*/

void mode4(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  int i = 0, r, g, b, w, r2, g2, b2, w2;
  float intensity, speed, ramp, duty, phase, balance;
  if (universe != config.universe)
    return;
  if (RGB && (length - config.offset) < 2 * 3 + 4)
    return;
  if (RGBW && (length - config.offset) < 2 * 4 + 4)
    return;
  r         = data[config.offset + i++];
  g         = data[config.offset + i++];
  b         = data[config.offset + i++];
  if (RGBW)
    w       = data[config.offset + i++];
  r2        = data[config.offset + i++];
  g2        = data[config.offset + i++];
  b2        = data[config.offset + i++];
  if (RGBW)
    w2      = data[config.offset + i++];
  intensity = data[config.offset + i++]  / 255.; // fraction between 0 and 1
  speed     = data[config.offset + i++]  / 8.;   // the value 256 maps onto 32 flashes per second
  ramp      = data[config.offset + i++] * (360. / 255.);
  duty      = data[config.offset + i++] * (360. / 255.);

  // the ramp cannot be too wide
  if (duty < 180)
    ramp = (ramp < duty ? ramp : duty);
  else
    ramp = (ramp < (360 - duty) ? ramp : (360 - duty));

  // determine the present phase in the cycle
  phase = (speed * millis()) * 360. / 1000.;
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
  w = BALANCE(balance, w, w2);

  // scale with the intensity
  r = intensity * r;
  g = intensity * g;
  b = intensity * b;
  w = intensity * w;

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    if (RGB)
      strip.setPixelColor(pixel, r, g, b);
    else if (RGBW)
      strip.setPixelColor(pixel, r, g, b, w);
  }
  strip.show();
}

/*
  mode 5: single color slider, segment that can be moved along the array (between the edges)
  channel 1 = red
  channel 2 = green
  channel 3 = blue
  channel 4 = white
  channel 5 = intensity
  channel 6 = position (from 0-255 or 0-360 degrees, relative to the length of the array)
  channel 7 = width    (from 0-255 or 0-360 degrees, relative to the length of the array)
*/

void mode5(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  int i = 0, r, g, b, w;
  float intensity, width, position, phase, balance;
  if (universe != config.universe)
    return;
  if (RGB && (length - config.offset) < 3 + 3)
    return;
  if (RGBW && (length - config.offset) < 4 + 3)
    return;
  r         = data[config.offset + i++];
  g         = data[config.offset + i++];
  b         = data[config.offset + i++];
  if (RGBW)
    w       = data[config.offset + i++];
  intensity = data[config.offset + i++] / 255.; // fraction between 0 and 1
  position  = data[config.offset + i++] * (strip.numPixels() - 1) / 255.;
  width     = data[config.offset + i++] * (strip.numPixels() - 0) / 255.;

  // scale with the intensity
  r = intensity * r;
  g = intensity * g;
  b = intensity * b;
  w = intensity * w;

  position -= strip.numPixels() / 2;
  position /= strip.numPixels() / 2;
  position *= (strip.numPixels() - width) / 2;
  position += strip.numPixels() / 2;

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {

    if (width > 0 && pixel >= ROUND(position - width / 2) && pixel <= ROUND(position + width / 2))
      balance = 1;
    else
      balance = 0;

    if (RGB)
      strip.setPixelColor(pixel, balance * r, balance * g, balance * b);
    else if (RGBW)
      strip.setPixelColor(pixel, balance * r, balance * g, balance * b, balance * w);
  }
  strip.show();
}

/*
  mode 6: dual color slider, segment can be moved along the array (between the edges)
  channel 1  = color 1 red
  channel 2  = color 1 green
  channel 3  = color 1 blue
  channel 4  = color 1 white
  channel 5  = color 2 red
  channel 6  = color 2 green
  channel 7  = color 2 blue
  channel 8  = color 2 white
  channel 9  = intensity
  channel 10 = position (from 0-255 or 0-360 degrees, relative to the length of the array)
  channel 11 = width    (from 0-255 or 0-360 degrees, relative to the length of the array)
*/

void mode6(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  int i = 0, r, g, b, w, r2, g2, b2, w2;
  float intensity, width, position, phase, balance;
  if (universe != config.universe)
    return;
  if (RGB && (length - config.offset) < 2 * 3 + 3)
    return;
  if (RGBW && (length - config.offset) < 2 * 4 + 3)
    return;
  r         = data[config.offset + i++];
  g         = data[config.offset + i++];
  b         = data[config.offset + i++];
  if (RGBW)
    w       = data[config.offset + i++];
  r2        = data[config.offset + i++];
  g2        = data[config.offset + i++];
  b2        = data[config.offset + i++];
  if (RGBW)
    w2      = data[config.offset + i++];
  intensity = data[config.offset + i++] / 255.; // fraction between 0 and 1
  position  = data[config.offset + i++] * (strip.numPixels() - 1) / 255.;
  width     = data[config.offset + i++] * (strip.numPixels() - 0) / 255.;

  position -= strip.numPixels() / 2;
  position /= strip.numPixels() / 2;
  position *= (strip.numPixels() - width) / 2;
  position += strip.numPixels() / 2;

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {

    if (width > 0 && pixel >= ROUND(position - width / 2) && pixel <= ROUND(position + width / 2))
      balance = 1;
    else
      balance = 0;

    if (RGB)
      strip.setPixelColor(pixel, intensity * BALANCE(balance, r, r2), intensity * BALANCE(balance, g, g2), intensity * BALANCE(balance, b, b2));
    else if (RGBW)
      strip.setPixelColor(pixel, intensity * BALANCE(balance, r, r2), intensity * BALANCE(balance, g, g2), intensity * BALANCE(balance, b, b2), intensity * BALANCE(balance, w, w2));
  }
  strip.show();
}

/*
  mode 7: single color smooth slider, segment can be moved along the array (continuous over the edge)
  channel 1 = red
  channel 2 = green
  channel 3 = blue
  channel 4 = white
  channel 5 = intensity
  channel 6 = position (from 0-255 or 0-360 degrees, relative to the length of the array)
  channel 7 = width    (from 0-255 or 0-360 degrees, relative to the length of the array)
  channel 8 = ramp     (from 0-255 or 0-360 degrees, relative to the length of the array)
*/

void mode7(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  int i = 0, r, g, b, w;
  float intensity, position, width, ramp;
  if (universe != config.universe)
    return;
  if (RGB && (length - config.offset) < 3 + 4)
    return;
  if (RGBW && (length - config.offset) < 4 + 4)
    return;
  r         = data[config.offset + i++];
  g         = data[config.offset + i++];
  b         = data[config.offset + i++];
  if (RGBW)
    w       = data[config.offset + i++];
  intensity = data[config.offset + i++] / 255.;        // fraction between 0 and 1
  position  = data[config.offset + i++] * 360 / 255.;
  width     = data[config.offset + i++] * 360 / 255.;
  ramp      = data[config.offset + i++] * 360 / 255.;

  // the ramp cannot be too wide
  if (width < 180)
    ramp = (ramp < width ? ramp : width);
  else
    ramp = (ramp < (360 - width) ? ramp : (360 - width));

  // scale with the intensity
  r = intensity * r;
  g = intensity * g;
  b = intensity * b;
  w = intensity * w;

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    float phase = 360. * pixel / (strip.numPixels() - 1);
    phase = WRAP180(phase - position);
    phase = ABS(phase);

    float balance = 0;
    if (width == 0)
      balance = 0;
    else if (phase < (width / 2. - ramp / 2.))
      balance = 1;
    else if (phase > (width / 2. + ramp / 2.))
      balance = 0;
    else if (ramp > 0)
      balance = ((width / 2. + ramp / 2.) - phase) / ramp;

    if (RGB)
      strip.setPixelColor(pixel, balance * r, balance * g, balance * b);
    else if (RGBW)
      strip.setPixelColor(pixel, balance * r, balance * g, balance * b, balance * w);
  }
  strip.show();
}

/*
  mode 8: dual color smooth slider, segment can be moved along the array (continuous over the edge)
  channel 1  = color 1 red
  channel 2  = color 1 green
  channel 3  = color 1 blue
  channel 4  = color 1 white
  channel 5  = color 2 red
  channel 6  = color 2 green
  channel 7  = color 2 blue
  channel 8  = color 2 white
  channel 9  = intensity
  channel 10 = position (from 0-255 or 0-360 degrees, relative to the length of the array)
  channel 11 = width    (from 0-255 or 0-360 degrees, relative to the length of the array)
  channel 12 = ramp     (from 0-255 or 0-360 degrees, relative to the length of the array)
*/

void mode8(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  int i = 0, r, g, b, w, r2, g2, b2, w2;
  float intensity, position, width, ramp;
  if (universe != config.universe)
    return;
  if (RGB && (length - config.offset) < 2 * 3 + 4)
    return;
  if (RGBW && (length - config.offset) < 2 * 4 + 4)
    return;
  r         = data[config.offset + i++];
  g         = data[config.offset + i++];
  b         = data[config.offset + i++];
  if (RGBW)
    w       = data[config.offset + i++];
  r2        = data[config.offset + i++];
  g2        = data[config.offset + i++];
  b2        = data[config.offset + i++];
  if (RGBW)
    w2      = data[config.offset + i++];
  intensity = data[config.offset + i++] / 255.;        // fraction between 0 and 1
  position  = data[config.offset + i++] * 360 / 255.;
  width     = data[config.offset + i++] * 360 / 255.;
  ramp      = data[config.offset + i++] * 360 / 255.;

  // the ramp cannot be too wide
  if (width < 180)
    ramp = (ramp < width ? ramp : width);
  else
    ramp = (ramp < (360 - width) ? ramp : (360 - width));

  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    float phase = 360. * pixel / (strip.numPixels() - 1);
    phase = WRAP180(phase - position);
    phase = ABS(phase);

    float balance = 0;
    if (width == 0)
      balance = 0;
    else if (phase < (width / 2. - ramp / 2.))
      balance = 1;
    else if (phase > (width / 2. + ramp / 2.))
      balance = 0;
    else if (ramp > 0)
      balance = ((width / 2. + ramp / 2.) - phase) / ramp;

    if (RGB)
      strip.setPixelColor(pixel, intensity * BALANCE(balance, r, r2), intensity * BALANCE(balance, g, g2), intensity * BALANCE(balance, b, b2));
    else if (RGBW)
      strip.setPixelColor(pixel, intensity * BALANCE(balance, r, r2), intensity * BALANCE(balance, g, g2), intensity * BALANCE(balance, b, b2), intensity * BALANCE(balance, w, w2));
  }
  strip.show();
}

void mode9(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  };

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

void mode10(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {};
void mode11(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {};
void mode12(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {};
void mode13(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {};
void mode14(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {};
void mode15(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {};
void mode16(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {};

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

void singleLed(byte r, byte g, byte b, byte w) {
  fullBlack();
  strip.setPixelColor(0, strip.Color(r, g, b, w ) );
  strip.show();
}

void singleRed() {
  singleLed(128, 0, 0, 0);
}
void singleGreen() {
  singleLed(0, 128, 0, 0);
}
void singleBlue() {
  singleLed(128, 0, 0, 0);
}
void singleYellow() {
  singleLed(128, 128, 0, 0);
}
void singleCyan() {
  singleLed(0, 128, 128, 0);
}
void singleMagenta() {
  singleLed(128, 0, 128, 0);
}

// helper functions to convert an integer into individual RGBW values
uint8_t red(uint32_t c) {
  return (c >> 8);
}
uint8_t green(uint32_t c) {
  return (c >> 16);
}
uint8_t blue(uint32_t c) {
  return (c);
}

// The colours transition from r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3, 0);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0, 0);
}


void fullRed() {
  Serial.println("fullRed");
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(255, 0, 0, 0 ) );
  }
  strip.show();
}

void fullGreen() {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0 ) );
  }
  strip.show();
}

void fullBlue() {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 255, 0 ) );
  }
  strip.show();
}

void fullWhite() {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0, 255 ) );
  }
  strip.show();
}

void fullBlack() {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0, 0 ) );
  }
  strip.show();
}

// Fill the dots one after the other with a specific color
void colorWipe(uint8_t wait, uint32_t c) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void pulseWhite(uint8_t wait) {
  for (int j = 0; j < 256 ; j++) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0, gamma_l[j] ) );
    }
    delay(wait);
    strip.show();
  }
  for (int j = 255; j >= 0 ; j--) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0, gamma_l[j] ) );
    }
    delay(wait);
    strip.show();
  }
}


void rainbowFade2White(uint8_t wait, int rainbowLoops, int whiteLoops) {
  float fadeMax = 100.0;
  int fadeVal = 0;
  uint32_t wheelVal;
  int redVal, greenVal, blueVal;
  for (int k = 0 ; k < rainbowLoops ; k ++) {
    for (int j = 0; j < 256; j++) { // 5 cycles of all colors on wheel
      for (int i = 0; i < strip.numPixels(); i++) {
        wheelVal = Wheel(((i * 256 / strip.numPixels()) + j) & 255);
        redVal = red(wheelVal) * float(fadeVal / fadeMax);
        greenVal = green(wheelVal) * float(fadeVal / fadeMax);
        blueVal = blue(wheelVal) * float(fadeVal / fadeMax);
        strip.setPixelColor( i, strip.Color( redVal, greenVal, blueVal ) );
      }
      //First loop, fade in!
      if (k == 0 && fadeVal < fadeMax - 1) {
        fadeVal++;
      }
      //Last loop, fade out!
      else if (k == rainbowLoops - 1 && j > 255 - fadeMax ) {
        fadeVal--;
      }
      strip.show();
      delay(wait);
    }

  }
  /*  delay(500);
    for (int k = 0 ; k < whiteLoops ; k ++) {
      for (int j = 0; j < 256 ; j++) {
        for (uint16_t i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(0, 0, 0, gamma_l[j] ) );
        }
        strip.show();
      }
      delay(2000);
      for (int j = 255; j >= 0 ; j--) {
        for (uint16_t i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(0, 0, 0, gamma_l[j] ) );
        }
        strip.show();
      }
    }
    delay(500);
  */
}


void whiteOverRainbow(uint8_t wait, uint8_t whiteSpeed, uint8_t whiteLength ) {
  if (whiteLength >= strip.numPixels()) whiteLength = strip.numPixels() - 1;
  int head = whiteLength - 1;
  int tail = 0;
  int loops = 3;
  int loopNum = 0;
  static unsigned long lastTime = 0;
  while (true) {
    for (int j = 0; j < 256; j++) {
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        if ((i >= tail && i <= head) || (tail > head && i >= tail) || (tail > head && i <= head) ) {
          strip.setPixelColor(i, strip.Color(0, 0, 0, 255 ) );
        }
        else {
          strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
        }
      }
      if (millis() - lastTime > whiteSpeed) {
        head++;
        tail++;
        if (head == strip.numPixels()) {
          loopNum++;
        }
        lastTime = millis();
      }
      if (loopNum == loops) return;
      head %= strip.numPixels();
      tail %= strip.numPixels();
      strip.show();
      delay(wait);
    }
  }
}


// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;
  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

