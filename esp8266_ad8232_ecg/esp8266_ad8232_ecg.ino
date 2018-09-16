/*
  This sketch is for an ESP8266 connected to an AD8232 ECG module+

  It uses WIFiManager for initial configuration and includes a web-interface
  that allows to monitor and change parameters.

  The status of the wifi connection, http server and received data
  is indicated by the builtin LED.
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>         // https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <WiFiUDP.h>
#include <Ticker.h>
#include <FS.h>

#include "setup_ota.h"
#include "blink_led.h"
#include "fieldtrip_buffer.h"

// this allows some sections of the code to be disabled for debugging purposes
#define ENABLE_WEBINTERFACE
#define ENABLE_MDNS
#define ENABLE_BUFFER

Ticker sampler;

const char* host = "AD8232-ECG";
const char* version = __DATE__ " / " __TIME__;

ESP8266WebServer server(80);
Config config;

// keep track of the timing of the web interface
long tic_web = 0;
long tic_buffer = 0;
long tic_heartbeat = 0;
int sample = 0, block = 0;
bool flush0 = false, flush1 = false;
uint16_t *block0 = NULL, *block1 = NULL;
int ftserver = 0;

#define FSAMPLE   1000
#define NCHANS    1
#define BLOCKSIZE 200
#define LO1 D6 // Lead-off detection
#define LO2 D7 // Lead-off detection

/************************************************************************************************/

void getSample() {
  if (sample == BLOCKSIZE) {
    // switch to the start of the other block
    Serial.println("flip");
    switch (block) {
      case 0:
        flush0 = true;
        block = 1;
        break;
      case 1:
        flush1 = true;
        block = 0;
        break;
    }
    sample = 0;
  }

  // get the current ECG value
  unsigned int value = 0;
  if (digitalRead(LO1))
    value = 0;
  else if (digitalRead(LO1))
    value = 0xFFFF;
  else
    value = analogRead(A0); // 10-bits ADC, value between 0 and 1023

  // store it in the active block
  switch (block) {
    case 0:
      block0[++sample] = value;
      break;
    case 1:
      block1[++sample] = value;
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
  initialConfig();
  Serial.println(config.address);
  Serial.println(config.port);

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
  // wifiManager.resetSettings();  // this is only needed when flashing a completely new ESP8266
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(host);
  Serial.println("Connected to WiFi");

  if (WiFi.status() == WL_CONNECTED)
    ledSlow();

#ifdef ENABLE_WEBINTERFACE
  // this serves all URIs that cannot be resolved to a file on the SPIFFS filesystem
  server.onNotFound(handleNotFound);

  server.on("/", HTTP_GET, []() {
    tic_web = millis();
    handleRedirect("/index");
  });

  server.on("/index", HTTP_GET, []() {
    tic_web = millis();
    handleStaticFile("/index.html");
  });

  server.on("/defaults", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleDefaults");
    handleStaticFile("/reload_success.html");
    delay(2000);
    ledFast();
    initialConfig();
    saveConfig();
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    WiFi.hostname(host);
    ESP.restart();
  });

  server.on("/reconnect", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleReconnect");
    handleStaticFile("/reload_success.html");
    delay(2000);
    ledFast();
    WiFiManager wifiManager;
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.startConfigPortal(host);
    Serial.println("connected");
    if (WiFi.status() == WL_CONNECTED)
      ledSlow();
  });

  server.on("/reset", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleReset");
    handleStaticFile("/reload_success.html");
    delay(2000);
    ledFast();
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
    CONFIG_TO_JSON(port, "port");

    root["version"] = version;
    root["uptime"]  = long(millis() / 1000);
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
#endif

#ifdef ENABLE_MDNS
  // announce the hostname and web server through zeroconf
  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);
#endif

#ifdef ENABLE_BUFFER
  Serial.println(config.address);
  Serial.println(config.port);

  if ((ftserver = fieldtrip_open_connection(config.address, config.port)) > 0)
    Serial.println("Connection to FieldTrip buffer server opened");
  else
    Serial.println("Connection to FieldTrip buffer server failed");
  if (fieldtrip_write_header(ftserver, DATATYPE_UINT16, NCHANS, FSAMPLE) == 0)
    Serial.println("Wrote header to FieldTrip buffer server");
  else
    Serial.println("Failed writing header to FieldTrip buffer server");
#endif

  if ((block0 = (uint16_t *)malloc(sizeof(uint16_t) * BLOCKSIZE)) == NULL) {
    Serial.println("Failed to allocate memory for block 0");
    ledFast();
  }
  if ((block1 = (uint16_t *)malloc(sizeof(uint16_t) * BLOCKSIZE)) == NULL) {
    Serial.println("Failed to allocate memory for block 1");
    ledFast();
  }

  // start sampling the ECG data
  sampler.attach_ms(1, getSample);

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
  if (flush0) {
    // write the full block to the remote FieldTrip buffer
    if (fieldtrip_write_data(ftserver, DATATYPE_UINT16, NCHANS, BLOCKSIZE, (byte *)block0) != 0)
      Serial.println("Failed writing data to FieldTrip buffer server");
    flush0 = false;
  }

  if (flush1) {
    // write the full block to the remote FieldTrip buffer
    if (fieldtrip_write_data(ftserver, DATATYPE_UINT16, NCHANS, BLOCKSIZE, (byte *)block1) != 0)
      Serial.println("Failed writing data to FieldTrip buffer server");
    flush1 = false;
  }
#endif

  return;
} // loop
