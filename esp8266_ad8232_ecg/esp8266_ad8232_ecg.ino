/*
  This sketch is for an ESP8266 connected to an AD8232 module to record the ECG
  and to send the continuous signal with the Fieldtrip buffer protocol to a server.

  It uses WIFiManager for initial configuration and includes a web-interface
  that allows to monitor and change parameters.

  The status of the wifi connection, http server and received data
  is indicated by the builtin LED.
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <Ticker.h>               // https://github.com/sstaub/Ticker

#include "webinterface.h"
#include "blink_led.h"
#include "fieldtrip_buffer.h"

// this allows some sections of the code to be disabled for debugging purposes
#define ENABLE_WEBINTERFACE
#define ENABLE_MDNS
#define ENABLE_BUFFER

const char* host = "AD8232-ECG";
const char* version = __DATE__ " / " __TIME__;

ESP8266WebServer server(80);
Ticker sampler;

#define ADC       A0
#define NCHANS    1
#define FSAMPLE   200
#define BLOCKSIZE (FSAMPLE/10)
#define LO1 D6 // Lead-off detection
#define LO2 D7 // Lead-off detection

long tic_web = 0;
int sample = 0, block = 0;
bool flush0 = false, flush1 = false;
uint16_t block0[BLOCKSIZE], block1[BLOCKSIZE];
int ftserver = 0, status = 0;
int lo1 = 0, lo2 = 0;

/************************************************************************************************/

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
    // sample the lead-off detection every block
    // they don't behave as they should
    lo1 = digitalRead(LO1);
    lo2 = digitalRead(LO2);
  }

  // get the current ECG value, it is from a 10-bits ADC, value between 0 and 1023
  uint16_t value = analogRead(ADC);

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

/************************************************************************************************/

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  Serial.print("\n[esp8266_ad8232 / ");
  Serial.print(__DATE__ " / " __TIME__);
  Serial.println("]");

  pinMode(LO1, INPUT); // Setup for leads off detection LO +
  pinMode(LO2, INPUT); // Setup for leads off detection LO -

  ledInit();

  WiFi.hostname(host);
  WiFi.begin();

  SPIFFS.begin();

  if (loadConfig()) {
    ledSlow();
    delay(1000);
  }
  else {
    ledFast();
    delay(1000);
  }
  Serial.println(config.address);
  Serial.println(config.port);

  if (WiFi.status() != WL_CONNECTED)
    ledFast();

  WiFiManager wifiManager;
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(host);
  Serial.println("connected");

  if (WiFi.status() == WL_CONNECTED)
    ledSlow();

#ifdef ENABLE_WEBINTERFACE
  // this serves all URIs that cannot be resolved to a file on the SPIFFS filesystem
  server.onNotFound(handleNotFound);

  server.on("/", HTTP_GET, []() {
    tic_web = millis();
    handleRedirect("/index.html");
  });

  server.on("/defaults", HTTP_GET, []() {
    tic_web = millis();
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
    tic_web = millis();
    Serial.println("handleReconnect");
    handleStaticFile("/reload_success.html");
    ledFast();
    server.close();
    server.stop();
    delay(5000);
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.startConfigPortal(host);
    Serial.println("connected");
    server.begin();
    if (WiFi.status() == WL_CONNECTED)
      ledSlow();
  });

  server.on("/restart", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleRestart");
    handleStaticFile("/reload_success.html");
    ledFast();
    server.close();
    server.stop();
    SPIFFS.end();
    delay(5000);
    ESP.restart();
  });

  server.on("/dir", HTTP_GET, [] {
    tic_web = millis();
    handleDirList();
  });

  server.on("/json", HTTP_PUT, [] {
    tic_web = millis();
    handleJSON();
  });

  server.on("/json", HTTP_POST, [] {
    tic_web = millis();
    handleJSON();
  });

  server.on("/json", HTTP_GET, [] {
    tic_web = millis();
    StaticJsonBuffer<300> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    S_CONFIG_TO_JSON(address, "address");
    N_CONFIG_TO_JSON(port, "port");
    root["version"] = version;
    root["uptime"]  = long(millis() / 1000);
    String str;
    root.printTo(str);
    server.setContentLength(str.length());
    server.send(200, "application/json", str);
  });

  server.on("/update", HTTP_GET, [] {
    tic_web = millis();
    handleStaticFile("/update.html");
  });

  server.on("/update", HTTP_POST, handleUpdate1, handleUpdate2);

  // start the web server
  server.begin();
#endif

#ifdef ENABLE_MDNS
  // announce the hostname and web server through zeroconf
  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);
#endif

#ifdef ENABLE_BUFFER
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

#endif

  // start sampling the ECG
  sampler.attach_ms(1000 / FSAMPLE, getSample);

  Serial.println("====================================================");
  Serial.println("Setup done");
  Serial.println("====================================================");

  return;
} // setup

/************************************************************************************************/

void loop() {

#ifdef ENABLE_WEBINTERFACE
  server.handleClient();
#endif

#ifdef ENABLE_BUFFER
  byte *ptr = NULL;

  if (flush0) {
    ptr = (byte *)block0;
    flush0 = false;
  }
  else if (flush1) {
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
      if (status == 0)
        Serial.println("Wrote data");
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

#endif

  delay(10);
  return;
} // loop
