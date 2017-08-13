/*
 *  This sketch serves as a 12 volt trigger to switch a NAD-D3020 audio amplifier on and off.
 *  The on and off state are controlled over a http call. The current status can also be probed.
 *
 *  The sketch is designed to run on a Wemos D1 mini that has pin 15 connected to a NPN transistor.
 *  Although it is designed for the 12V trigger input, the output voltage can be toggled
 *  between 0 and 5 volt. This is enough to trigget the amplifier.
 *
 *  Valid HTTP calls are
 *    http://nad-d3020.local/on
 *    http://nad-d3020.local/off
 *    http://nad-d3020.local/status   this returns on or off
 *    http://nad-d3020.local/version
 *    http://nad-d3020.local/reset
 *    http://nad-d3020.local/         this returns an identifier
 *
 *  (C) 2017 Robert Oostenveld
 *
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "secret.h"

#define PINOUT 14 // this corresponds to D5 on the Wemos D1 mini

// These are defined in secret.h
// const char* ssid     = "xxxx";
// const char* password = "xxxx";

const char* version = __DATE__ " / " __TIME__;

ESP8266WebServer server(80);

void handleRoot() {
  Serial.println("/");
  server.send(200, "text/plain", "NAD-D3020");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

long checkpoint = millis();

void setup(void) {
  pinMode(PINOUT, OUTPUT);
  digitalWrite(PINOUT, LOW);

  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  Serial.print("\n[esp8266_12v_trigger / ");
  Serial.print(version);
  Serial.println("]");

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (count++>10) {
      Serial.println("reset");
      ESP.reset();
    }
    else
      Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("NAD-D3020")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/on", []() {
    Serial.println("/on");
    digitalWrite(PINOUT, HIGH);
    server.send(200, "text/plain", "on");
  });

  server.on("/off", []() {
    Serial.println("/off");
    digitalWrite(PINOUT, LOW);
    server.send(200, "text/plain", "off");
  });

  server.on("/status", []() {
    Serial.println("/status");
    if (digitalRead(PINOUT))
      server.send(200, "text/plain", "on");
    else
      server.send(200, "text/plain", "off");
  });

  server.on("/version", []() {
    Serial.println("/version");
    server.send(200, "text/plain", version);
  });

  server.on("/reset", []() {
    Serial.println("/reset");
    server.send(200, "text/plain", "reset");
    ESP.reset();
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
} // setup

void loop(void) {
  // check the connection status every 10 seconds
  if ((millis()-checkpoint) > 10000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("reset");
      ESP.reset();
    }
    else
      checkpoint = millis();
    }
  // process incoming http requests
  server.handleClient();
} // loop
