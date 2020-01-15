/*
 *  This sketch serves as a 12 volt trigger to switch a NAD-D3020 audio amplifier on and off.
 *  The on and off state are controlled over a http call. The current status can also be probed.
 *
 *  The GPOI output of pin D6 is used to switch a PC900v optocoupler. Its output voltage can be 
 *  toggled between 0 and 5 volt, which is is enough to trigger the amplifier.
 *
 *  See https://robertoostenveld.nl/12-volt-trigger-for-nad-d3020-amplifier/
 *
 *  Valid HTTP calls are
 *    http://nad-d3020.local/on
 *    http://nad-d3020.local/off
 *    http://nad-d3020.local/status   this returns on or off
 *    http://nad-d3020.local/version
 *    http://nad-d3020.local/reset
 *    http://nad-d3020.local/         this returns an identifier
 *
 *  (C) 2017-2019 Robert Oostenveld
 *
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#define PINOUT D6

const char* version = __DATE__ " / " __TIME__;
const char* name = "NAD-D3020";

ESP8266WebServer server(80);

void handleRoot() {
  Serial.println("/");
  server.send(200, "text/plain", name);
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
  digitalWrite(PINOUT, HIGH); // note that the logic is inverted

  Serial.begin(115200);
  while (!Serial) {
    ;
  }
 
  Serial.print("\n[esp8266_12v_trigger / ");
  Serial.print(version);
  Serial.println("]");

  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  wifiManager.autoConnect(name);

  Serial.println("");
  Serial.println("Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(name)) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/on", []() {
    Serial.println("/on");
    digitalWrite(PINOUT, LOW); // note that the logic is inverted
    server.send(200, "text/plain", "on");
  });

  server.on("/off", []() {
    Serial.println("/off");
    digitalWrite(PINOUT, HIGH); // note that the logic is inverted
    server.send(200, "text/plain", "off");
  });

  server.on("/status", []() {
    Serial.println("/status");
    if (digitalRead(PINOUT)) // note that the logic is inverted
      server.send(200, "text/plain", "off");
    else
      server.send(200, "text/plain", "on");
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
