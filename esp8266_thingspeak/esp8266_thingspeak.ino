#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "secret.h"

/* these are defined in secret.h
   const char* SSID   = "XXXXXXXXXXXXXXXX";
   const char* PASS   = "XXXXXXXXXXXXXXXX";
   const char* APIKEY = "XXXXXXXXXXXXXXXX"; // Write API Key for my ThingSpeak Channel
*/

const char* version    = __DATE__ " / " __TIME__;

WiFiClient client;

const char* server = "api.thingspeak.com";
const int postingInterval = 20 * 1000; // post data every 20 seconds
unsigned int resetCounter = 0;

/*****************************************************************************/

void setup () {
  Serial.begin(115200);
  Serial.print("\n[esp2866_thingspeak / ");
  Serial.print(version);
  Serial.println("]");

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  Serial.println("");

  // Wait for connection
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    if (count++ > 20) {
      Serial.println("No WiFi connection, restart...");
      ESP.restart();
    }
    else
      Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
} // setup

/*****************************************************************************/

void loop() {
  if (client.connect(server, 80)) {

    // Measure Signal Strength (RSSI) of Wi-Fi connection
    long rssi = WiFi.RSSI();
    Serial.print("RSSI: ");
    Serial.println(rssi);

    // Construct API request body
    String body = "field1=";
    body += String(rssi);

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: " + String(server) + "\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + String(APIKEY) + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: " + String(body.length()) + "\n");
    client.print("\n");
    client.print(body);
    client.print("\n");

    Serial.println("------------------------------------------------------------");
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
    }
    Serial.println("------------------------------------------------------------");

    client.stop();

    // wait and then post again
    delay(postingInterval);
    resetCounter = 0;

  }
  else {
    Serial.println("Connection Failed.");
    Serial.println();

    resetCounter++;

    if (resetCounter >= 5 ) {
      ESP.restart();
    }
  }
} // loop
