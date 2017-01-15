/*
  This example will receive multiple universes via Artnet and control a
  strip of ws2811 leds via Adafruit's NeoPixel library:

  https://github.com/rstephan/ArtnetWifi
  https://github.com/adafruit/Adafruit_NeoPixel
*/

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>
#include <ArtnetWifi.h>
#include <Adafruit_NeoPixel.h>
#include <FS.h>

#include "setup_ota.h"
#include "neopixel_mode.h"

Config config;
ESP8266WebServer server(80);
const char* host = "ARTNET";
float temperature = 0;

// Neopixel settings
const byte dataPin = D2;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, dataPin, NEO_GRBW + NEO_KHZ800); // start with one pixel

// Artnet settings
ArtnetWifi artnet;
unsigned int packetCounter = 0;

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
long tic_loop = 0;
long tic_fps = 0;
long tic_packet = 0;
long frameCounter = 0;

//this will be called for each UDP packet received
void onDmxPacket(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  // print some feedback
  Serial.print("packetCounter = ");
  Serial.print(packetCounter++);
  if ((millis() - tic_fps) > 1000 && frameCounter > 100) {
    // don't estimate the FPS too quickly
    Serial.print(",  FPS = ");
    Serial.print(1000 * frameCounter / (millis() - tic_fps));
    tic_fps = millis();
    frameCounter = 0;
  }
  Serial.println();

  // copy the data from the UDP packet over to the global universe buffer
  global.universe = universe;
  global.sequence = sequence;
  if (length < 512)
    global.length = length;
  for (int i = 0; i < global.length; i++)
    global.data[i] = data[i];
} // onDmxpacket

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

  global.universe = 0;
  global.sequence = 0;
  global.length = 512;
  global.data = (uint8_t *)malloc(512);

  SPIFFS.begin();
  loadConfig();

  //  Serial.print("universe   = ");
  //  Serial.println(config.universe);
  //  Serial.print("offset     = ");
  //  Serial.println(config.offset);
  //  Serial.print("length     = ");
  //  Serial.println(config.length);
  //  Serial.print("leds       = ");
  //  Serial.println(config.leds);
  //  Serial.print("white      = ");
  //  Serial.println(config.white);
  //  Serial.print("brightness = ");
  //  Serial.println(config.brightness);
  //  Serial.print("hsv        = ");
  //  Serial.println(config.hsv);
  //  Serial.print("mode       = ");
  //  Serial.println(config.mode);
  //  Serial.print("reverse    = ");
  //  Serial.println(config.reverse);
  //  Serial.print("speed      = ");
  //  Serial.println(config.speed);
  //  Serial.print("position   = ");
  //  Serial.println(config.position);

  strip.begin();
  updateNeopixelStrip();
  strip.setBrightness(255);
  singleYellow();
  delay(1000);

  WiFiManager wifiManager;
  // wifiManager.resetSettings();
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(host);
  Serial.println("connected");

  // this serves all URIs that can be resolved to a file on the SPIFFS filesystem
  server.onNotFound(handleNotFound);

  server.on("/", HTTP_GET, []() {
    handleRedirect("/index");
  });

  server.on("/index", HTTP_GET, []() {
    handleStaticFile("/index.html");
  });

  server.on("/wifi", HTTP_GET, []() {
    handleStaticFile("/reload_success.html");
    Serial.println("handleWifi");
    Serial.flush();
    WiFiManager wifiManager;
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.startConfigPortal(host);
    Serial.println("connected");
  });

  server.on("/reset", HTTP_GET, []() {
    handleStaticFile("/reload_success.html");
    Serial.println("handleReset");
    Serial.flush();
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
  });

  server.on("/dir", HTTP_GET, handleDirList);

  server.on("/monitor", HTTP_GET, [] {
    handleStaticFile("/monitor.html");
  });

  server.on("/hello", HTTP_GET, [] {
    handleStaticFile("/hello.html");
  });

  server.on("/settings", HTTP_GET, [] {
    handleStaticFile("/settings.html");
  });

  server.on("/json", HTTP_PUT, handleJSON);

  server.on("/json", HTTP_POST, handleJSON);

  server.on("/json", HTTP_GET, [] {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    CONFIG_TO_JSON(universe, "universe");
    CONFIG_TO_JSON(offset, "offset");
    CONFIG_TO_JSON(length, "length");
    CONFIG_TO_JSON(leds, "leds");
    CONFIG_TO_JSON(white, "white");
    CONFIG_TO_JSON(brightness, "brightness");
    CONFIG_TO_JSON(hsv, "hsv");
    CONFIG_TO_JSON(mode, "mode");
    CONFIG_TO_JSON(reverse, "reverse");
    CONFIG_TO_JSON(speed, "speed");
    CONFIG_TO_JSON(position, "position");
    root["packets"]     = packetCounter; // this is dynamic
    root["temperature"] = temperature;  // this is dynamic
    String str;
    root.printTo(str);
    server.send(200, "application/json", str);
  });

  server.on("/update", HTTP_GET, []() {
    handleStaticFile("/update.html");
  });

  server.on("/update", HTTP_POST, handleUpdate1, handleUpdate2);

  server.begin();

  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);

  if (WiFi.status() == WL_CONNECTED)
    singleGreen();

  artnet.begin();
  artnet.setArtDmxCallback(onDmxPacket);

  // initialize all timers
  tic_loop   = millis();
  tic_packet = millis();
  tic_fps    = millis();
} // setup

void loop() {
  server.handleClient();
  artnet.read();

  // this section gets executed at a maximum rate of around 10Hz
  if ((millis() - tic_loop) > 99)
    updateNeopixelStrip();

  // this section gets executed at a maximum rate of around 100Hz
  if ((millis() - tic_loop) > 9) {
    if (WiFi.status() != WL_CONNECTED) {
      // show the WiFi status
      singleRed();
    }
    else if (config.mode >= 0 && config.mode < (sizeof(mode) / 4)) {
      tic_loop = millis();
      frameCounter++;
      // call the function corresponding to the current mode
      (*mode[config.mode]) (global.universe, global.length, global.sequence, global.data);
    }
  }
} // loop


