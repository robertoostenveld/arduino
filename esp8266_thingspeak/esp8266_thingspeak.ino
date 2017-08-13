#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "secret.h"

/* these are defined in secret.h
   const char* SSID         = "XXXXXXXXXXXXXXXX";
   const char* PASS         = "XXXXXXXXXXXXXXXX";
   const char* writeAPIKey  = "XXXXXXXXXXXXXXXX"; // Write API Key for my ThingSpeak Channel
 */

const char* version    = __DATE__ " / " __TIME__;

WiFiClient client;

// ThingSpeak Settings
const char* server = "api.thingspeak.com";
const int postingInterval = 20 * 1000; // post data every 20 seconds

/*****************************************************************************/

void setup () {
  Serial.begin(115200);
  Serial.print("\n[esp2866_thingspeak / ");
  Serial.print(__DATE__);
  Serial.print(" / ");
  Serial.print(__TIME__);
  Serial.println("]");

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  Serial.println("");

  // Wait for connection
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    if (count++>20) {
      Serial.println("No WiFi connection, reset...");
      ESP.reset();
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

    // Construct API request body
    String body = "field1=";
           body += String(rssi);
    
    Serial.print("RSSI: ");
    Serial.println(rssi); 

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + String(writeAPIKey) + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(body.length());
    client.print("\n\n");
    client.print(body);
    client.print("\n\n");

  }
  client.stop();

  // wait and then post again
  delay(postingInterval);
} // loop
