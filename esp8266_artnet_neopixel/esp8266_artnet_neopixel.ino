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
  mode0, mode1, mode2, mode3, mode4
};

//this will be called for each packet received
void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  Serial.print("msgCounter = ");
  Serial.println(msgCounter++);

  if (config.mode >= 0 && config.mode < (sizeof(update) / 4)) {
    // call the function corresponding to the current mode
    long tic = millis();
    (*update[config.mode]) (universe, length, sequence, data);
    Serial.print("updating mode ");
    Serial.print("takes ");
    Serial.print(millis() - tic);
    Serial.println(" ms");
  }
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

//  colorWipe(20, strip.Color(255, 0, 0));    // Red
//  colorWipe(20, strip.Color(0, 255, 0));    // Green
//  colorWipe(20, strip.Color(0, 0, 255));    // Blue
//  colorWipe(20, strip.Color(0, 0, 0, 255)); // White
//  colorWipe(20, strip.Color(0, 0, 0, 0));   // Black

  // give the WiFi status, only until the first Artnet package arrives
  fullBlack();
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


