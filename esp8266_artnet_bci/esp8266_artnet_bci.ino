#include <ESP8266WiFi.h>         // https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArtnetWifi.h>          // https://github.com/rstephan/ArtnetWifi
#include <Adafruit_NeoPixel.h>   // https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library

extern "C" {
#include "user_interface.h"      // https://github.com/willemwouters/ESP8266/wiki/Timer-example
}

const char* host = "ARTNET-BCI";
const char* version = __DATE__ " / " __TIME__;

#define UNIVERSE  1
#define OFFSET    0
#define NUMPIXELS 1
#define PIN       5
#define NUMEL(x)  (sizeof(x) / sizeof((x)[0]))
#define WRAP(x,y) (x>=y ? x-y : x)

WiFiManager wifiManager;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ800);
ESP8266WebServer server(80);
static ETSTimer sequenceTimer;

// Artnet settings
ArtnetWifi artnet;
unsigned long packetCounter = 0;

// keep track of the time just after a http request
long tic_web = 0;

// Blink sequence settings
byte enable = 0, step = 0, current = 0;
byte r = 255, g = 255, b = 255;

unsigned int sequence0[] = {125, 125};  // default, rapid blinking at 4Hz
unsigned int sequence1[] = {100, 0};    // constant on
unsigned int sequence2[] = {0, 100};    // constant off
unsigned int sequence3[] = {8, 8};      // approximately 60Hz
unsigned int sequence4[] = {80, 16, 96, 96, 16, 48, 48, 80, 48, 64, 32, 128, 80, 112, 96, 80, 48, 48, 80, 80, 48, 64, 32, 80, 96, 96, 48, 48, 96, 64, 96, 32, 48, 112, 112, 80, 48, 32, 96, 112, 144, 16, 48, 128, 64, 80, 80, 48, 48, 64, 112, 64, 112, 80, 128, 32, 48, 96, 64, 80, 64, 16, 32, 64, 64, 112, 128, 0, 112, 112, 32, 80, 144, 128, 112, 64, 16, 112, 48, 64, 64, 96, 64, 96, 80, 48, 80, 80, 64, 80, 64, 16, 64, 80, 96, 96, 48, 80, 64, 96};
unsigned int sequence5[] = {80, 100, 60, 60, 100, 80, 100, 100, 80, 100, 140, 60, 120, 160, 120, 160, 40, 80, 100, 200, 200, 160, 60, 80, 120, 100, 80, 160, 80, 180, 80, 80, 160, 100, 100, 100, 40, 60, 140, 160, 100, 80, 100, 100, 20, 60, 60, 100, 160, 100, 180, 200, 60, 220, 120, 60, 120, 100, 160, 80, 100, 160, 80, 140, 60, 140, 20, 100, 120, 100, 80, 60, 100, 60, 140, 140, 40, 140, 80, 100, 140, 180, 60, 100, 200, 140, 140, 60, 60, 80, 80, 60, 80, 40, 80, 60, 80, 220, 160, 80};
unsigned int sequence6[] = {96, 96, 256, 224, 96, 96, 256, 224, 96, 192, 64, 96, 64, 128, 160, 224, 128, 352, 160, 64, 160, 160, 32, 128, 96, 288, 128, 128, 128, 224, 160, 192, 64, 128, 32, 96, 192, 96, 160, 96, 96, 128, 192, 192, 256, 192, 192, 192, 96, 32, 160, 96, 64, 64, 192, 96, 32, 224, 320, 32, 96, 256, 224, 128, 128, 224, 192, 192, 192, 96, 96, 160, 192, 224, 128, 128, 96, 128, 192, 160, 224, 128, 192, 96, 96, 352, 256, 96, 128, 160, 288, 256, 224, 224, 160, 256, 96, 192, 128, 224};

#define NUMSEQUENCE 7
unsigned int *sequence[NUMSEQUENCE] = {sequence0, sequence1, sequence2, sequence3, sequence4, sequence5, sequence6};
unsigned int length[NUMSEQUENCE] = {2, 2, 2, 2, 100, 100, 100};

/***********************************************************************************************/

//this is perpetually scheduled for the blink sequence updating
void sequence_update() {
  os_timer_disarm(&sequenceTimer);

  step = WRAP(step, length[current]);
  unsigned int interval = sequence[current][step];

  if (interval == 0 ) {
    // skip to the next step
    step++;
    step = WRAP(step, length[current]);
    interval = sequence[current][step];
  }

  if (enable) {
    if (step % 2 == 0)
      singleBright();
    else
      singleBlack();
  }

  step++;

  os_timer_arm(&sequenceTimer, interval, 0);
} // sequence_update

/***********************************************************************************************/

//this will be called for each UDP packet received
void packet_receive(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  Serial.print("received packet = ");
  Serial.println(packetCounter++);

  if (universe != UNIVERSE)
    return;

  // shift to the specified channels in the DMX packet
  data += OFFSET;
  length -= OFFSET;

  // 0 = current sequence
  if (length > 0) {
    if (data[0] >= 0 && data[0] < NUMSEQUENCE) {
      current = data[0];
      step = 0;
    }
  }

  // 1, 2, 3 = color
  if (length > 3) {
    r = data[1];
    g = data[2];
    b = data[3];
  }
} // packet_receive

/***********************************************************************************************/

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  Serial.println("setup starting");

  pixels.setBrightness(150);
  pixels.begin();

  current = 0;

  if (WiFi.status() != WL_CONNECTED)
    singleRed();

  wifiManager.autoConnect(host);

  if (WiFi.status() == WL_CONNECTED) {
    singleGreen();
    Serial.println("connected");
  }

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(host);
  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA");
    server = ESP8266WebServer(); // See https://github.com/esp8266/Arduino/issues/686
    artnet = ArtnetWifi();
    os_timer_disarm(&sequenceTimer);
    singleBlue();
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA");
    ESP.reset();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if      (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  server.onNotFound([]() {
    tic_web = millis();
    Serial.println("handleNotFound");
    server.send(404, "text/plain", "not found");
  });

  server.on("/", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleRoot");
    server.send(200, "text/plain", host);
  });

  server.on("/reconnect", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleReconnect");
    singleRed();
    server.stop();  // switch the web server off
    wifiManager.startConfigPortal(host);
    server.begin();  // switch the web server back on
    singleGreen();
    Serial.println("connected");
  });

  server.on("/reset", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleReset");
    wifiManager.resetSettings();
    singleRed();
    ESP.reset();
  });

  // start the web server
  server.begin();

  // start the OTA server
  ArduinoOTA.begin();

  // announce the web server through zeroconf
  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);

  // start the Artnet server
  artnet.begin();
  artnet.setArtDmxCallback(packet_receive);

  // start the timer
  os_timer_disarm(&sequenceTimer);
  os_timer_setfn(&sequenceTimer, (os_timer_func_t *) sequence_update, NULL);
  os_timer_arm(&sequenceTimer, 1000, 0);

  Serial.println("setup done");

} // setup

/***********************************************************************************************/

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    enable = 0;
    singleRed();
  }
  else if ((millis() - tic_web) < 1000) {
    enable = 0;
    singleBlue();
  }
  else  {
    enable = 1;
    server.handleClient();
    artnet.read();
    ArduinoOTA.handle();
    // the remainder of the work gets done by the timer and callback functions
  }

} // loop

/***********************************************************************************************/

void singleRed() {
  pixels.setPixelColor(0, 255, 0, 0);
  pixels.show();
}

void singleGreen() {
  pixels.setPixelColor(0, 0, 255, 0);
  pixels.show();
}

void singleBlue() {
  pixels.setPixelColor(0, 0, 0, 255);
  pixels.show();
}

void singleYellow() {
  pixels.setPixelColor(0, 255, 255, 0);
  pixels.show();
}

void singleBlack() {
  pixels.setPixelColor(0, 0, 0, 0);
  pixels.show();
}

void singleWhite() {
  pixels.setPixelColor(0, 255, 255, 255);
  pixels.show();
}

void singleBright() {
  pixels.setPixelColor(0, r, g, b);
  pixels.show();
}
