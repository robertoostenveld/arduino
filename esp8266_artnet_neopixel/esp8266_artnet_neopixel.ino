/*
  This example will receive multiple universes via Artnet and control a
  strip of ws2811 leds via Adafruit's NeoPixel library:

  https://github.com/rstephan/ArtnetWifi
  https://github.com/adafruit/Adafruit_NeoPixel
*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArtnetWifi.h>
#include <ConfigManager.h>
#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "mode.h"

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

// Configuration settings
Config config;
ConfigManager configManager;

// Neopixel settings
const byte dataPin = D2;
byte brightness = 100;
int numberOfChannels;     // total number of channels you want to receive
Adafruit_NeoPixel strip = Adafruit_NeoPixel(0, dataPin, NEO_GRBW + NEO_KHZ800);

// Artnet settings
ArtnetWifi artnet;
unsigned int msgCounter = 0;

// use an array of function pointers to jump to the desired mode
void (*update[]) (uint16_t, uint16_t, uint8_t, uint8_t *) {
  mode0, mode1, mode2, mode3
};

//this will be called for each packet received
void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  Serial.print("msgCounter = ");
  Serial.println(msgCounter++);

  if (config.mode >= 0 && config.mode < (sizeof(update) / 4))
    // call the function corresponding to the current mode
    (*update[config.mode]) (universe, length, sequence, data);
  else
    // the current mode does not map onto a function
    Serial.println("invalid mode");
} // onDmxFrame

void setup() {
  Serial.begin(115200);
  Serial.println("setup");

  delay(500);
  singleYellow();
  delay(500);

  configManager.setAPName("ARTNET");
  configManager.setAPFilename("/index.html");
  configManager.addParameter("length",   &config.length); // number of pixels
  configManager.addParameter("leds",     &config.leds);   // number of leds per pixel: 3 for NEO_GRB, 4 for NEO_GRBW
  configManager.addParameter("universe", &config.universe);
  configManager.addParameter("offset",   &config.offset);
  configManager.addParameter("mode",     &config.mode);
  configManager.begin(config);

  Serial.print("length   = ");
  Serial.println(config.length);
  Serial.print("leds     = ");
  Serial.println(config.leds);
  Serial.print("universe = ");
  Serial.println(config.universe);
  Serial.print("offset   = ");
  Serial.println(config.offset);
  Serial.print("mode     = ");
  Serial.println(config.mode);

  // update the neopixel strip configuration
  numberOfChannels = config.length * config.leds;
  strip.updateLength(config.length);
  if (config.leds == 3)
    strip.updateType(NEO_GRB + NEO_KHZ800);
  else if (config.leds == 4)
    strip.updateType(NEO_GRBW + NEO_KHZ800);

  strip.setBrightness(brightness);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  /*
    colorWipe(20, strip.Color(255, 0, 0));    // Red
    colorWipe(20, strip.Color(0, 255, 0));    // Green
    colorWipe(20, strip.Color(0, 0, 255));    // Blue
    colorWipe(20, strip.Color(0, 0, 0, 255)); // White
    colorWipe(20, strip.Color(0, 0, 0, 0));   // Black
  */

  // give the WiFi status, only until the first Artnet package arrives
  if (WiFi.status() != WL_CONNECTED)
    singleRed();
  else
    singleGreen();

  artnet.begin();
  artnet.setArtDmxCallback(onDmxFrame);
} // setup

void loop() {
  configManager.loop();
  artnet.read();
} // loop

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

