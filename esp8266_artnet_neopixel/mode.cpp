#include "config.h"
#include "mode.h"

extern Config config;
extern Adafruit_NeoPixel strip;

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
  int r, g, b, w, intensity;
  if (universe != config.universe)
    return;
  if (RGB && (length - config.offset) < 3 + 1)
    return;
  if (RGBW && (length - config.offset) < 4 + 1)
    return;
  if (RGB) {
    r = data[config.offset + 0];
    g = data[config.offset + 1];
    b = data[config.offset + 2];
    intensity = data[config.offset + 3];
  }
  else if (RGBW) {
    r = data[config.offset + 0];
    g = data[config.offset + 1];
    b = data[config.offset + 2];
    w = data[config.offset + 3];
    intensity = data[config.offset + 4];
  }
  // scale with the intensity
  r = (r * intensity) / 255;
  g = (g * intensity) / 255;
  b = (b * intensity) / 255;
  w = (w * intensity) / 255;
  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    if (RGB)
      strip.setPixelColor(pixel, r, g, b);
    else if (RGBW)
      strip.setPixelColor(pixel, r, g, b, w);
  }
  strip.show();
} // mode1

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
  channel 9  = balance (between color 1 and color2)
  channel 10 = intensity (this also allows to black out the colors)
*/

void mode2(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  int r, g, b, w, r2, g2, b2, w2, balance, intensity;
  if (universe != config.universe)
    return;
  if (RGB && (length - config.offset) < 6 + 2)
    return;
  if (RGBW && (length - config.offset) < 8 + 2)
    return;
  if (RGB) {
    r  = data[config.offset + 0];
    g  = data[config.offset + 1];
    b  = data[config.offset + 2];
    r2 = data[config.offset + 3];
    g2 = data[config.offset + 4];
    b2 = data[config.offset + 5];
    balance   = data[config.offset + 6];
    intensity = data[config.offset + 7];
  }
  else if (RGBW) {
    r  = data[config.offset + 0];
    g  = data[config.offset + 1];
    b  = data[config.offset + 2];
    w  = data[config.offset + 3];
    r2 = data[config.offset + 4];
    g2 = data[config.offset + 5];
    b2 = data[config.offset + 6];
    w2 = data[config.offset + 7];
    balance   = data[config.offset + 8];
    intensity = data[config.offset + 9];
  }
  // apply the balance between the two colors
  r = (r * (255 - balance) + r2 * balance) / 255;
  g = (g * (255 - balance) + g2 * balance) / 255;
  b = (b * (255 - balance) + b2 * balance) / 255;
  w = (w * (255 - balance) + w2 * balance) / 255;
  // scale with the intensity
  r = (r * intensity) / 255;
  g = (g * intensity) / 255;
  b = (b * intensity) / 255;
  w = (w * intensity) / 255;
  for (int pixel = 0; pixel < strip.numPixels(); pixel++) {
    if (RGB)
      strip.setPixelColor(pixel, r, g, b);
    else if (RGBW)
      strip.setPixelColor(pixel, r, g, b, w);
  }
  strip.show();
} // mode2

/*
  mode 3: single uniform color, blinking between the color and black
  channel 1 = red
  channel 2 = green
  channel 3 = blue
  channel 4 = white
  channel 5 = intensity
  channel 6 = speed (number of flashes per unit of time)
  channel 7 = ramp (to specify whether there is a abrubt or more smooth transition)
  channel 8 = duty cycle (what is the time ratio between color and black)
*/

void mode3(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
}

/*
  mode 4: uniform color, blinking between color 1 and color 2
  channel 1  = color 1 red
  channel 2  = color 1 green
  channel 3  = color 1 blue
  channel 4  = color 1 white
  channel 5  = color 1 intensity
  channel 6  = color 2 red
  channel 7  = color 2 green
  channel 8  = color 2 blue
  channel 9  = color 2 white
  channel 10 = intensity
  channel 11 = speed
  channel 12 = ramp (here it is the time it takes to transition, i.e color transitions can be abrupt, but also gradually fused)
  channel 13 = duty cycle
*/

void mode4(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
}

/************************************************************************************/
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

