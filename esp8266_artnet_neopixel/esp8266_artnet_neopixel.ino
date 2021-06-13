/*
  This sketch receive a DMX universes via Artnet to control a
  strip of ws2811 leds via Adafruit's NeoPixel library:

  https://github.com/rstephan/ArtnetWifi
  https://github.com/adafruit/Adafruit_NeoPixel
*/

#include <ESP8266WiFi.h>         // https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>
#include <ArtnetWifi.h>          // https://github.com/rstephan/ArtnetWifi 
#include <Adafruit_NeoPixel.h>
#include <FS.h>

#include "setup_ota.h"
#include "neopixel_mode.h"

Config config;
ESP8266WebServer server(80);
const char* host = "ARTNET";
const char* version = __DATE__ " / " __TIME__;
float temperature = 0, fps = 0;

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
long tic_loop = 0, tic_fps = 0, tic_packet = 0, tic_web = 0;
long frameCounter = 0;

//this will be called for each UDP packet received
void onDmxPacket(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  // print some feedback
  Serial.print("packetCounter = ");
  Serial.print(packetCounter++);
  if ((millis() - tic_fps) > 1000 && frameCounter > 100) {
    // don't estimate the FPS too quickly
    fps = 1000 * frameCounter / (millis() - tic_fps);
    tic_fps = millis();
    frameCounter = 0;
    Serial.print(",  FPS = ");
    Serial.print(fps);
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
  strip.updateLength(config.pixels);
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
  while (!Serial) {
    ;
  }
  Serial.println("setup starting");

  global.universe = 0;
  global.sequence = 0;
  global.length = 512;
  global.data = (uint8_t *)malloc(512);

  SPIFFS.begin();
  strip.begin();

  initialConfig();

  if (loadConfig()) {
    updateNeopixelStrip();
    strip.setBrightness(255);
    singleYellow();
    delay(1000);
  }
  else {
    updateNeopixelStrip();
    strip.setBrightness(255);
    singleRed();
    delay(1000);
  }

  WiFiManager wifiManager;
  // wifiManager.resetSettings();
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(host);
  Serial.println("connected");

  if (WiFi.status() == WL_CONNECTED)
    singleGreen();

  // this serves all URIs that can be resolved to a file on the SPIFFS filesystem
  server.onNotFound(handleNotFound);

  server.on("/", HTTP_GET, []() {
    tic_web = millis();
    handleRedirect("/index");
  });

  server.on("/index", HTTP_GET, []() {
    tic_web = millis();
    handleStaticFile("/index.html");
  });

  server.on("/version", HTTP_GET, []() {
    Serial.println("version");
    server.send(200, "text/plain", version);
  });
  
  server.on("/defaults", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleDefaults");
    handleStaticFile("/reload_success.html");
    delay(2000);
    singleRed();
    initialConfig();
    saveConfig();
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
  });

  server.on("/reconnect", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleReconnect");
    handleStaticFile("/reload_success.html");
    delay(2000);
    singleYellow();
    WiFiManager wifiManager;
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.startConfigPortal(host);
    Serial.println("connected");
    if (WiFi.status() == WL_CONNECTED)
      singleGreen();
  });

  server.on("/reset", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleReset");
    handleStaticFile("/reload_success.html");
    delay(2000);
    singleRed();
    ESP.restart();
  });

  server.on("/monitor", HTTP_GET, [] {
    tic_web = millis();
    handleStaticFile("/monitor.html");
  });

  server.on("/hello", HTTP_GET, [] {
    tic_web = millis();
    handleStaticFile("/hello.html");
  });

  server.on("/settings", HTTP_GET, [] {
    tic_web = millis();
    handleStaticFile("/settings.html");
  });

  server.on("/dir", HTTP_GET, [] {
    tic_web = millis();
    handleDirList();
  });

  server.on("/json", HTTP_PUT, [] {
    Serial.println("HTTP_PUT /json");
    tic_web = millis();
    handleJSON();
  });

  server.on("/json", HTTP_POST, [] {
    Serial.println("HTTP_POST /json");
    tic_web = millis();
    handleJSON();
  });

  server.on("/json", HTTP_GET, [] {
    Serial.println("HTTP_GET /json");
    tic_web = millis();
    StaticJsonBuffer<300> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    CONFIG_TO_JSON(universe, "universe");
    CONFIG_TO_JSON(offset, "offset");
    CONFIG_TO_JSON(pixels, "pixels");
    CONFIG_TO_JSON(leds, "leds");
    CONFIG_TO_JSON(white, "white");
    CONFIG_TO_JSON(brightness, "brightness");
    CONFIG_TO_JSON(hsv, "hsv");
    CONFIG_TO_JSON(mode, "mode");
    CONFIG_TO_JSON(reverse, "reverse");
    CONFIG_TO_JSON(speed, "speed");
    CONFIG_TO_JSON(split, "split");
    root["version"] = version;
    root["uptime"]  = long(millis() / 1000);
    root["packets"] = packetCounter;
    root["fps"]     = fps;
    String str;
    root.printTo(str);
    server.send(200, "application/json", str);
  });

  server.on("/update", HTTP_GET, [] {
    tic_web = millis();
    handleStaticFile("/update.html");
  });

  server.on("/update", HTTP_POST, handleUpdate1, handleUpdate2);

  // start the web server
  server.begin();

  // announce the hostname and web server through zeroconf
  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);

  artnet.begin();
  artnet.setArtDmxCallback(onDmxPacket);

  // initialize all timers
  tic_loop   = millis();
  tic_packet = millis();
  tic_fps    = millis();
  tic_web    = 0;

  Serial.println("setup done");
} // setup

void loop() {
  server.handleClient();

  if (WiFi.status() != WL_CONNECTED) {
    singleRed();
  }
  else if ((millis() - tic_web) < 5000) {
    singleBlue();
  }
  else  {
    artnet.read();

    // this section gets executed at a maximum rate of around 1Hz
    if ((millis() - tic_loop) > 999)
      updateNeopixelStrip();

    // this section gets executed at a maximum rate of around 100Hz
    if ((millis() - tic_loop) > 9) {
      if (config.mode >= 0 && config.mode < (sizeof(mode) / 4)) {
        // call the function corresponding to the current mode
        (*mode[config.mode]) (global.universe, global.length, global.sequence, global.data);
        tic_loop = millis();
        frameCounter++;
      }
    }
  }

  delay(1);
} // loop
