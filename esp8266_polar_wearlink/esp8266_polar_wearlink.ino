/*
  This sketch is for an ESP8266 connected to the Polar WearLink+
  receiver that comes with the Adafruit Polar T34 starter pack
  https://www.adafruit.com/product/1077.

  It uses WIFiManager for initial configuration and includes a web-interface
  that allows to monitor and change parameters.

  The status of the wifi connection, http server and received data
  is indicated by an RDB led that blinks Red, Green or Blue.
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>         // https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include <Redis.h>               // https://github.com/remicaumette/esp8266-redis

#include "webinterface.h"
#include "rgb_led.h"

// this allows some sections of the code to be disabled for debugging purposes
#define ENABLE_WEBINTERFACE
#define ENABLE_MDNS
#define ENABLE_REDIS
#define ENABLE_INTERRUPT

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
long tic_heartbeat = 0;

bool pulse = 0;
float bpm = 0;

#define INTERRUPT_PIN       13  // GPIO13 maps to pin D7
#define INTERRUPT_DEBOUNCE  200 // milliseconds
volatile byte interruptCounter = 0;

/************************************************************************************************/

void interruptHandler() {
  // prevent roll-around
  if (interruptCounter < 255)
    interruptCounter++;
}

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

#ifdef ENABLE_INTERRUPT
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), interruptHandler, RISING);
#endif

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
    handleRedirect("/index.html");
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
    S_CONFIG_TO_JSON(redis, "redis");
    N_CONFIG_TO_JSON(port, "port");
    N_CONFIG_TO_JSON(duration, "duration");

    root["heartrate"] = bpm;
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
  Serial.print("duration = ");
  Serial.println(config.duration);

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
  if ((millis() - tic_redis) > 5000) {
    tic_redis = millis();
    String str(tic_redis);
    char buf[32];
    str.toCharArray(buf, 32);

    if (redis_p != NULL)
      redis_p->set("polar.millis", buf);
  }
#endif

#ifdef ENABLE_INTERRUPT
  if (interruptCounter) {
    if ((millis() - tic_heartbeat) > INTERRUPT_DEBOUNCE) {
      long now = millis();
      ledMagenta();
      // skip the first beat, since the BPM cannot be computed
      if (tic_heartbeat > 0) {
        bpm = 60000. / (now - tic_heartbeat);
        String str(bpm);
        char buf[32];
        str.toCharArray(buf, 32);

        if (redis_p != NULL) {
          redis_p->publish("polar.heartbeat", buf);
          redis_p->set("polar.heartrate", buf);
          redis_p->set("polar.heartbeat", "1");
          pulse = 1;
        }
      }
      Serial.print("Heartbeat, BPM = ");
      Serial.println(bpm);
      tic_heartbeat = now;
    } // if debounce
    interruptCounter = 0;
  } // if interruptCounter

  if (pulse && ((millis() - tic_heartbeat) > config.duration)) {
    pulse = 0;
    if (redis_p != NULL) {
      redis_p->set("polar.heartbeat", "0");
    }
  }

  if ((millis() - tic_heartbeat) > 100) {
    // switch the LED back to the normal status color
    if ((millis() - tic_web) < 5000)
      ledBlue();
    else if ((WiFi.status() == WL_CONNECTED) && (redis_p != NULL))
      ledGreen();
    else if ((WiFi.status() == WL_CONNECTED) && (redis_p == NULL))
      ledCyan();
    else
      ledRed();
  }
#endif

}
