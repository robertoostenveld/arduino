#if not defined(ESP32)
#error This is a sketch for an ESP32 board, like the NodeMCU 32S, LOLIN32 Lite, or the Adafruit Huzzah32
#endif

#include <Arduino.h>
#include <ArduinoJson.h>  // https://arduinojson.org
#include <WiFi.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>

#include "webinterface.h"

/*********************************************************************************/

const char* host = "ESP32";
const char* version = __DATE__ " / " __TIME__;

Config config;
WebServer server(80);
WiFiManager wifiManager;

/*********************************************************************************/

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  Serial.println("Setup starting");

  // The SPIFFS file system contains the config.json, and the html and javascript code for the web interface
  SPIFFS.begin();
  loadConfig();
  printConfig();

  WiFiManager wifiManager;
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(host);
  Serial.println("connected");

  // this serves all URIs that can be resolved to a file on the SPIFFS filesystem
  server.onNotFound(handleNotFound);

  server.on("/", HTTP_GET, []() {
    handleRedirect("/index.html");
  });

  server.on("/defaults", HTTP_GET, []() {
    Serial.println("handleDefaults");
    handleStaticFile("/reload_success.html");
    defaultConfig();
    printConfig();
    saveConfig();
    server.close();
    server.stop();
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
    server.begin();
    if (WiFi.status() == WL_CONNECTED)
      Serial.println("WiFi status ok");
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
    DynamicJsonDocument root(300);
    N_CONFIG_TO_JSON(var1, "var1");
    N_CONFIG_TO_JSON(var2, "var2");
    N_CONFIG_TO_JSON(var3, "var3");
    root["version"] = version;
    root["uptime"] = long(millis() / 1000);
    String str;
    serializeJson(root, str);
    server.send(200, "application/json", str);
  });

  // start the web server
  server.begin();

  // announce the hostname and web server through zeroconf
  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);

  Serial.println("Setup done");
}

/*********************************************************************************/

void loop() {
  server.handleClient();
  delay(10);  // in milliseconds
}
