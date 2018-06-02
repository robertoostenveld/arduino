/*
  This sketch is for an ESP8266 connected to the Polar WearLink+
  receiver that comes with the Adafruit Polar T34 starter pack
  https://www.adafruit.com/product/1077.

  It uses WIFiManager for initial configuration and includes a web-interface
  that allows to monitor and change parameters.

  The status of the wifi connection, http server and received data
  is indicated by a RDB led that blinks Red, Green or Blue.
*/

#include <Arduino.h>
#include <Wire.h>
#include <Redis.h>               // https://github.com/remicaumette/esp8266-redis
#include <ESP8266WiFi.h>         // https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <WiFiUDP.h>
#include <FS.h>

#include "setup_ota.h"
#include "rgb_led.h"

// this allows some sections of the code to be disabled for debugging purposes
#define ENABLE_WEBINTERFACE
#define ENABLE_MDNS
#define ENABLE_REDIS

// pointer to object is needed because the initialization is delayed
Redis *redis_p = NULL;

// this port and address are not used, they are taken from the config instead
#define REDIS_ADDR      "127.0.0.1"
#define REDIS_PORT      6379
#define REDIS_PASSWORD  ""

Config config;
ESP8266WebServer server(80);
const char* host = "POLAR-WEARLINK";
const char* version = __DATE__ " / " __TIME__;

// keep track of the timing of the web interface
long tic_web = 0;
long tic_redis = 0;

/************************************************************************************************/

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  Serial.print("\n[esp8266_polar_wearlink / ");
  Serial.print(__DATE__ " / " __TIME__);
  Serial.println("]");

  WiFi.hostname(host);
  WiFi.begin();

  SPIFFS.begin();

  ledInit();

  if (loadConfig()) {
    ledYellow();
    delay(1000);
  }
  else {
    ledRed();
    delay(1000);
  }

  if (WiFi.status() != WL_CONNECTED)
    ledRed();

  WiFiManager wifiManager;
  // wifiManager.resetSettings();  // this is only needed when flashing a completely new ESP8266
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(host);
  Serial.println("Connected to WiFi");

  if (WiFi.status() == WL_CONNECTED)
    ledGreen();

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
    ledRed();
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
    ledRed();
    WiFiManager wifiManager;
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.startConfigPortal(host);
    Serial.println("connected");
    if (WiFi.status() == WL_CONNECTED)
      ledGreen();
  });

  server.on("/reset", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleReset");
    handleStaticFile("/reload_success.html");
    delay(2000);
    ledRed();
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
    CONFIG_TO_JSON(decay, "decay");
    S_CONFIG_TO_JSON(redis, "redis");
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

#ifdef ENABLE_REDIS
  // pointer to object is needed because the initialization is delayed
  Serial.print("redis = ");
  Serial.println(config.redis);
  Serial.print("port = ");
  Serial.println(config.port);

  Redis redis(config.redis, config.port);
  redis.setNoDelay(false);
  redis.setTimeout(100);

  if (redis.begin(REDIS_PASSWORD)) {
    Serial.println("Connected to the Redis server!");
    redis_p = &redis;
  }
  else {
    Serial.println("Failed to connect to the Redis server!");
    redis_p = NULL;
  }
#endif

  Serial.println("====================================================");
  Serial.println("Setup done");
  Serial.println("====================================================");

  while (true) {
    // somehow the redis connection fails when control moves from setup() to loop()
    // hence remain in the setup() function
    loop();
  }
}

/************************************************************************************************/

void loop() {
#ifdef ENABLE_WEBINTERFACE
  server.handleClient();
#endif

#ifdef ENABLE_REDIS
  if ((millis() - tic_redis) > 1000) {
    tic_redis = millis();
    String str(tic_redis);
    char buf[32];
    str.toCharArray(buf, 32);

    if (redis_p != NULL)
      redis_p->set("foo", buf);
  }
#endif
}
