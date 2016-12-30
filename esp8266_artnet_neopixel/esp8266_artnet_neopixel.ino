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
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, dataPin, NEO_GRBW + NEO_KHZ800); // start with one pixel

// Artnet settings
ArtnetWifi artnet;
unsigned int msgCounter = 0;

// Global universe buffer
struct {
  uint16_t universe;
  uint16_t length;
  uint8_t sequence;
  uint8_t *data;
} global;

// use an array of function pointers to jump to the desired mode
void (*mode[]) (uint16_t, uint16_t, uint8_t, uint8_t *) {
  mode0, mode1, mode2, mode3, mode4, mode5, mode6, mode7, mode8, mode9, mode10, mode11, mode12, mode13, mode14, mode15, mode16
};

// keep the timing of the function calls
long tic_loop = 0, tic_dmx = 0;
long executed = 0;

//this will be called for each UDP packet received
void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  // print some feedback
  Serial.print("msgCounter = ");
  Serial.print(msgCounter++);
  if ((millis() - tic_dmx) > 1000 && executed > 100) {
    // don't estimate the FPS too quickly
    Serial.print(",  FPS = ");
    Serial.print(1000 * executed / (millis() - tic_dmx));
    tic_dmx = millis();
    executed = 0;
  }
  Serial.println();

  // copy the data from the UDP packet over to the global universe buffer
  global.universe = universe;
  global.sequence = sequence;
  if (length < 512)
    global.length = length;
  for (int i = 0; i < global.length; i++)
    global.data[i] = data[i];
} // onDmxFrame

void updateNeopixelStrip(void) {
  // update the neopixel strip configuration
  strip.updateLength(config.length);
  strip.setBrightness(config.brightness);
  if (config.leds == 3)
    strip.updateType(NEO_GRB + NEO_KHZ800);
  else if (config.leds == 4 && config.white)
    strip.updateType(NEO_GRBW + NEO_KHZ800);
  else if (config.leds == 4 && !config.white)
    strip.updateType(NEO_GRBW + NEO_KHZ800);
}

void setup() {
  Serial.begin(115200);
  Serial.println("setup");

  strip.begin();
  strip.setBrightness(255);
  singleYellow();
  delay(1000);

  global.universe = 0;
  global.sequence = 0;
  global.length = 512;
  global.data = (uint8_t *)malloc(512);

  configManager.setAPName("ARTNET");
  configManager.setAPFilename("/index.html");
  configManager.addParameter("universe",   &config.universe);
  configManager.addParameter("offset",     &config.offset);
  configManager.addParameter("length",     &config.length); // number of pixels
  configManager.addParameter("leds",       &config.leds);   // number of leds per pixel: 3 for NEO_GRB, 4 for NEO_GRBW
  configManager.addParameter("white",      &config.white);  // if true: use the white led
  configManager.addParameter("brightness", &config.brightness);
  configManager.addParameter("hsv",        &config.hsv);    // if true: use HSV instead of RGB
  configManager.addParameter("mode",       &config.mode);
  configManager.addParameter("reverse",    &config.reverse);
  configManager.addParameter("speed",      &config.speed);
  configManager.addParameter("position",   &config.position);
  configManager.begin(config);

  Serial.print("universe   = ");
  Serial.println(config.universe);
  Serial.print("offset     = ");
  Serial.println(config.offset);
  Serial.print("length     = ");
  Serial.println(config.length);
  Serial.print("leds       = ");
  Serial.println(config.leds);
  Serial.print("white      = ");
  Serial.println(config.white);
  Serial.print("brightness = ");
  Serial.println(config.brightness);
  Serial.print("hsv        = ");
  Serial.println(config.hsv);
  Serial.print("mode       = ");
  Serial.println(config.mode);
  Serial.print("reverse    = ");
  Serial.println(config.reverse);
  Serial.print("speed      = ");
  Serial.println(config.speed);
  Serial.print("position   = ");
  Serial.println(config.position);

  updateNeopixelStrip();

  // show the WiFi status, only until the first Artnet package arrives
  fullBlack();
  if (WiFi.status() != WL_CONNECTED)
    singleRed();
  else
    singleGreen();

  artnet.begin();
  artnet.setArtDmxCallback(onDmxFrame);

  // reset all timers
  tic_loop  = millis();
  tic_dmx   = millis();
  tic_frame = millis();
} // setup

void loop() {
  configManager.loop();
  artnet.read();

  // this code gets executed at a maximum rate of 100Hz
  if ((millis() - tic_loop) > 9) {
    updateNeopixelStrip();

    if (config.mode >= 0 && config.mode < (sizeof(mode) / 4)) {
      tic_loop = millis();
      executed++;
      // call the function corresponding to the current mode
      (*mode[config.mode]) (global.universe, global.length, global.sequence, global.data);
    }
  }
} // loop


