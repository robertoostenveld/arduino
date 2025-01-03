#if not defined(ESP32)
#error This is a sketch for an ESP32 board, like the NodeMCU 32S, LOLIN32 Lite, or the Adafruit Huzzah32
#endif

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Ticker.h>               // https://github.com/sstaub/Ticker
#include <FS.h>
#include <SPIFFS.h>

#include "fieldtrip_buffer.h"
#include "rgb_led.h"
#include "webinterface.h"

#ifndef ARDUINOJSON_VERSION
#error ArduinoJson version 5 not found, please include ArduinoJson.h in your .ino file
#endif

#if ARDUINOJSON_VERSION_MAJOR != 5
#error ArduinoJson version 5 is required
#endif

WebServer server(80);
Ticker sampler;

// when SERIAL_PLOTTER is defined the ADC value is printed for each sample and some of the other debugging output is disabled
#define SERIAL_PLOTTER

#define ADC       33              // https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
#define NCHANS    1
#define FSAMPLE   250
#define BLOCKSIZE (FSAMPLE/10)

const char* host = "EXGPILL";
const char* version = __DATE__ " / " __TIME__;

unsigned int sample = 0, block = 0, total = 0;
bool flush0 = false, flush1 = false;
uint16_t block0[BLOCKSIZE], block1[BLOCKSIZE];
int ftserver = 0, status = 0;

void getSample() {
  if (sample == BLOCKSIZE) {
    // switch to the start of the other block
    switch (block) {
      case 0:
        sample = 0;
        flush0 = true;
        block = 1;
        break;
      case 1:
        sample = 0;
        flush1 = true;
        block = 0;
        break;
    }
  }

  // get the current ExG value, it is a 12-bits ADC with a value between 0 and 4095
  uint16_t value = analogRead(ADC);
#ifdef SERIAL_PLOTTER
  Serial.println(value);
#endif

  // store it in the active block
  switch (block) {
    case 0:
      block0[sample++] = value;
      break;
    case 1:
      block1[sample++] = value;
      break;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  Serial.println("");
  Serial.println("Setup start");

  ledInit();
  ledRed();

  SPIFFS.begin();
  loadConfig();

  WiFiManager wifiManager;
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(host);
  Serial.println("connected");
  ledGreen();

  // this serves all URIs that can be resolved to a file on the SPIFFS filesystem
  server.onNotFound(handleNotFound);

  server.on("/", HTTP_GET, []() {
    handleRedirect("/index.html");
  });

  server.on("/defaults", HTTP_GET, []() {
    Serial.println("handleDefaults");
    handleStaticFile("/reload_success.html");
    defaultConfig();
    saveConfig();
    server.close();
    server.stop();
    delay(5000);
    ESP.restart();
  });

  server.on("/reconnect", HTTP_GET, []() {
    Serial.println("handleReconnect");
    handleStaticFile("/reload_success.html");
    server.close();
    server.stop();
    delay(5000);
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.startConfigPortal(host);
    Serial.println("connected");
    server.begin();
  });

  server.on("/restart", HTTP_GET, []() {
    Serial.println("handleRestart");
    handleStaticFile("/reload_success.html");
    server.close();
    server.stop();
    SPIFFS.end();
    delay(5000);
    ESP.restart();
  });

  server.on("/json", HTTP_PUT, handleJSON);

  server.on("/json", HTTP_POST, handleJSON);

  server.on("/json", HTTP_GET, [] {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["address"] = config.address;
    root["port"]    = config.port;
    root["total"]   = total;
    root["version"] = version;
    root["uptime"]  = long(millis() / 1000);
    String content;
    root.printTo(content);
    server.send(200, "application/json", content);
  });

  // ask server to track these headers
  const char * headerkeys[] = {"User-Agent", "Content-Type"} ;
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize );

  server.begin();

  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);

  Serial.print("Connecting to ");
  Serial.print(config.address);
  Serial.print(" on port ");
  Serial.println(config.port);

  ftserver = fieldtrip_open_connection(config.address, config.port);
  if (ftserver > 0) {
    Serial.println("Connection opened");
    status = fieldtrip_write_header(ftserver, DATATYPE_UINT16, NCHANS, FSAMPLE);
    if (status == 0)
      Serial.println("Wrote header");
    else
      Serial.println("Failed writing header");
  }
  else {
    Serial.println("Failed opening connection");
  }

  // start sampling the ExG
  sampler.attach_ms(1000 / FSAMPLE, getSample);

  Serial.println("Setup done");
}

void loop() {
  server.handleClient();

  byte *ptr = NULL;
  if (flush0) {
    // block0 is ready to be sent
    ptr = (byte *)block0;
    flush0 = false;
  }
  else if (flush1) {
    // block1 is ready to be sent
    ptr = (byte *)block1;
    flush1 = false;
  }

  if (ptr) {
    if (ftserver == 0) {
      ftserver = fieldtrip_open_connection(config.address, config.port);
      if (ftserver > 0)
        Serial.println("Connection opened");
      else
        Serial.println("Failed opening connection");
    }

    if (ftserver > 0) {
      status = fieldtrip_write_data(ftserver, DATATYPE_UINT16, NCHANS, BLOCKSIZE, ptr);
      if (status == 0) {
        total += BLOCKSIZE;
#ifndef SERIAL_PLOTTER
        Serial.print("Wrote ");
        Serial.print(total);
        Serial.println(" samples");
#endif
      }
      else {
        Serial.println("Failed writing data");
        status = fieldtrip_close_connection(ftserver);
        if (status == 0)
          Serial.println("Connection closed");
        else
          Serial.println("Failed closing connection");
        ftserver = 0;
      }
    }
  }

  delay(10); // in milliseconds
}
