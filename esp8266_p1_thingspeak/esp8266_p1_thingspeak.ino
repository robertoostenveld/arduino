/*
   This sketch is designed for the P1 port of an ISKRA AM500 smart energy meter.
   It runs on a Wemos D1 mini that reads and parses the energy usage data and
   subsequently sends it to http://thingspeak.com/

   Pin 5 appears to behave as an https://en.wikipedia.org/wiki/Open_collector and
   hence requires to be connected over a ~1kOhm resistor  to VCC of the ESP8266.

   The schematic below shows how I wired the 6 pins of the RJ12 connector to
   the Wemos D1 mini.

   1---PWR_VCC-
   2-------RTS-------------------D1
   3---DAT_GND-------------------GND
   4--------NC-       +----------D2 = SoftwareSerial RX
   5--------TX--------|---R670---3V3
   6---PWR_GND-

*/

#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <dsmr.h>             // https://github.com/matthijskooijman/arduino-dsmr
#include "secret.h"

/* these are defined in secret.h
   const char* SSID   = "XXXXXXXXXXXXXXXX";
   const char* PASS   = "XXXXXXXXXXXXXXXX";
   const char* APIKEY = "XXXXXXXXXXXXXXXX"; // Write API Key for my ThingSpeak Channel
*/

const char* version    = __DATE__ " / " __TIME__;
const byte rxPin = D2;
const byte requestPin = D1;

#define BUFSIZE 1024

SoftwareSerial MySerial (rxPin, -1, true, BUFSIZE);
P1Reader reader(&MySerial, requestPin);
WiFiClient client;

const char* server = "api.thingspeak.com";
const unsigned long intervalTime = 20 * 1000; // post data every 20 seconds
unsigned long resetCounter = 0, lastTime = 0;
unsigned long lastLed = 0, durationLed = 1000;

using MyData = ParsedData <
               //             /* String */ identification,
               /* String */ p1_version,
               /* String */ timestamp,
               /* String */ equipment_id,
               /* FixedValue */ energy_delivered_tariff1,
               /* FixedValue */ energy_delivered_tariff2,
               /* FixedValue */ energy_returned_tariff1,
               /* FixedValue */ energy_returned_tariff2,
               /* String */ electricity_tariff,
               /* FixedValue */ power_delivered,
               /* FixedValue */ power_returned,
               /* FixedValue */ electricity_threshold,
               /* uint8_t */ electricity_switch_position,
               /* uint32_t */ electricity_failures,
               /* uint32_t */ electricity_long_failures,
               //             /* String */ electricity_failure_log,
               /* uint32_t */ electricity_sags_l1,
               /* uint32_t */ electricity_sags_l2,
               /* uint32_t */ electricity_sags_l3,
               /* uint32_t */ electricity_swells_l1,
               /* uint32_t */ electricity_swells_l2,
               /* uint32_t */ electricity_swells_l3,
               /* String */ message_short,
               /* String */ message_long,
               /* FixedValue */ voltage_l1,
               /* FixedValue */ voltage_l2,
               /* FixedValue */ voltage_l3,
               /* FixedValue */ current_l1,
               /* FixedValue */ current_l2,
               /* FixedValue */ current_l3,
               /* FixedValue */ power_delivered_l1,
               /* FixedValue */ power_delivered_l2,
               /* FixedValue */ power_delivered_l3,
               /* FixedValue */ power_returned_l1,
               /* FixedValue */ power_returned_l2,
               /* FixedValue */ power_returned_l3,
               /* uint16_t */ gas_device_type,
               /* String */ gas_equipment_id,
               /* uint8_t */ gas_valve_position,
               /* TimestampedFixedValue */ gas_delivered,
               /* uint16_t /*/ thermal_device_type,
               /* String */ thermal_equipment_id,
               /* uint8_t */ thermal_valve_position,
               /* TimestampedFixedValue */ thermal_delivered,
               /* uint16_t */ water_device_type,
               /* String */ water_equipment_id,
               /* uint8_t */ water_valve_position,
               /* TimestampedFixedValue */ water_delivered,
               /* uint16_t */ slave_device_type,
               /* String */ slave_equipment_id,
               /* uint8_t */ slave_valve_position,
               /* TimestampedFixedValue */ slave_delivered
               >;

MyData data;

/*****************************************************************************/

struct Printer {
  template<typename Item>
  void apply(Item &i) {
    if (i.present()) {
      Serial.print(Item::name);
      Serial.print(F(": "));
      Serial.print(i.val());
      Serial.print(Item::unit());
      Serial.println();
    }
  }
};

/*****************************************************************************/

void setup() {
  Serial.begin(115200);
  MySerial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println();
  Serial.print("\n[esp2866_p1_thingspeak / ");
  Serial.print(version);
  Serial.println("]");

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  delay(500);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //Upon restart, fire off a one-off reading
  reader.enable(true);
  lastTime = millis();
}

/*****************************************************************************/

void loop () {
  // Allow the reader to check the serial buffer regularly
  reader.loop();

  // Every now and then, fire off a one-off reading
  unsigned long now = millis();
  if (now - lastTime > intervalTime) {
    reader.enable(true);
    lastTime = now;
  }

  // Switch the LED off, it is switched on when sending a http message
  if (now - lastLed > durationLed) {
    digitalWrite(LED_BUILTIN, HIGH);  // it is inverted on the ESP12F
    lastLed = now;
  }

  if (reader.available()) {
    // A complete telegram has arrived
    Serial.println("---raw telegram-------------------------------------------------");
    Serial.println(reader.raw());

    String err;
    if (reader.parse(&data, &err)) {
      // Parse succesful, print result
      Serial.println("---parse succesful---------------------------------------------");
      data.applyEach(Printer());
      sendThingspeak();
    } else {
      // Parser error, print error
      Serial.println("---parse error-------------------------------------------------");
      Serial.println(err);
    } // parse result ok
  } // full telegram available
} // loop

/*****************************************************************************/

void sendThingspeak() {
  if (client.connect(server, 80)) {
    digitalWrite(LED_BUILTIN, LOW); // it is inverted on the ESP12F
    lastLed = millis();

    // Construct API request body
    String body = "field1=";
    body += String(data.power_delivered);
    body += String("&field2=");
    body += String(data.power_returned);
    body += String("&field3=");
    body += String(data.gas_delivered);
    body += String("&field4=");
    body += String(data.energy_delivered_tariff1);
    body += String("&field5=");
    body += String(data.energy_delivered_tariff2);

    Serial.println("---post url----------------------------------------------------");
    Serial.println(body);

    // Send message to thingspeak
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: " + String(server) + "\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + String(APIKEY) + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: " + String(body.length()) + "\n");
    client.print("\n");
    client.print(body);
    client.print("\n");

    Serial.println("---server response---------------------------------------------");
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
    }

    client.stop();
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
}
