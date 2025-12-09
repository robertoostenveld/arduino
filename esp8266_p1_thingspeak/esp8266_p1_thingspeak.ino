/*
   This sketch is designed for the P1 port of an ISKRA AM500 smart energy meter.
   It runs on a Wemos D1 mini PRO that reads and parses the energy usage data and
   subsequently sends it to http://thingspeak.com/

   Pin 5 appears to behave as an https://en.wikipedia.org/wiki/Open_collector and
   hence requires to be connected over a ~1kOhm resistor  to VCC of the ESP8266.

   The Wemos D1 mini PRO includes a lithium battery charging circuit, which I 
   connected to a small LiPo battery. This allows running it from the low power that
   the P1 port provides, as the battery takes care of the current spikes during 
   wifi transmission. The battery needed to be fully charged prior to connecting it 
   to the P1 port, otherwise too much current would be drawn and the P1 port would 
   switch off and on repeatedly.

   See http://domoticx.com/p1-poort-slimme-meter-hardware/ and
   http://domoticx.com/p1-poort-slimme-meter-hardware/

   The schematic below shows how I wired the 6 pins of the RJ12 connector (on the left) 
   to the Wemos D1 mini PRO (on the right).

   1---PWR_VCC-------------------5V
   2-------RTS-------------------D1
   3---DAT_GND-------------------GND
   4--------NC-       +----------D2 = SoftwareSerial RX
   5--------TX--------|---R670---3V3
   6---PWR_GND-------------------GND

*/

#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>   // https://github.com/plerup/espsoftwareserial/
#include <PubSubClient.h>     // https://github.com/knolleary/pubsubclient
#include <dsmr.h>             // https://github.com/matthijskooijman/arduino-dsmr
#include "secret.h"

/* these are defined in secret.h 
  for the wifi
    #define SSID    "XXXXXXXXXXXXXXXX"
    #define PASS    "XXXXXXXXXXXXXXXX"
  for thingspeak mqtt
    #define CLIENTID "XXXXXXXXXXXXXXXX"
    #define USERNAME "XXXXXXXXXXXXXXXX"
    #define PASSWORD "XXXXXXXXXXXXXXXX"
  for thingspeak http
    #define CHANNEL "XXXXXXXXXXXXXXXX"
    #define APIKEY  "XXXXXXXXXXXXXXXX"
*/

const char* version    = __DATE__ " / " __TIME__;
const int8_t rxPin = D2;
const int8_t txPin = -1;
const int8_t requestPin = D1;

#define BUFSIZE 1024

// one of these options is to be selected: LOCAL is a local mqtt broker, MQTT is thingspeak mqtt, REST is thingspeak http
// #define USE_LOCAL
#define USE_MQTT
// #define USE_REST

SoftwareSerial MySerial;
P1Reader reader(&MySerial, requestPin);
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

#if defined USE_LOCAL
const char* mqtt_server = "192.168.1.16";
const int mqtt_port = 1883;
#elif defined USE_MQTT
const char* mqtt_server = "mqtt3.thingspeak.com";
const int mqtt_port = 1883;
#elif defined USE_REST
// this uses the server and port defined below
#endif

const char* server = "api.thingspeak.com";
const int port = 80;

const unsigned long intervalTime = 20 * 1000; // post data every 20 seconds
const unsigned long durationLed = 1000;       // the status LED remains on for 1 second
unsigned long resetCounter = 0, lastLed = 0, lastTime = 0;

using MyData = ParsedData <
               /* String */ identification,
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
               /* String */ electricity_failure_log,
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

MyData *globaldata;

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
  MySerial.begin(115200, SWSERIAL_8N1, rxPin, txPin, true, BUFSIZE);

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

#if defined USE_LOCAL
  mqtt_client.setServer(mqtt_server, mqtt_port);
#elif defined USE_MQTT
  mqtt_client.setServer(mqtt_server, mqtt_port);
#endif

  //Upon restart, fire off a one-off reading
  reader.enable(true);
  lastTime = millis();
}

/*****************************************************************************/

void loop () {
  // this object should be recreated on every call, otherwise the parsing fails
  MyData localdata;
  // share a pointer to the object to the sendThingspeak functions
  globaldata = &localdata;

  // Allow the reader to check the serial buffer regularly
  reader.loop();

  // Allow the MQTT client to do its things
  mqtt_client.loop();

  // Fire off a one-off reading if there is no message for some time
  unsigned long now = millis();
  if (now - lastTime > intervalTime) {
    reader.enable(true);
    lastTime = now;
  }

  // Switch the LED off, it was switched on when sending a HTTP or MQTT message
  if (now - lastLed > durationLed) {
    digitalWrite(LED_BUILTIN, HIGH);  // it is inverted
    lastLed = now;
  }

  if (reader.available()) {
    // A complete telegram has arrived
    Serial.println("---raw telegram-------------------------------------------------");
    Serial.println(reader.raw());
    lastTime = now;

    String err;
    if (reader.parse(&localdata, &err)) {
      // Parse succesful, print result
      Serial.println("---parse succesful---------------------------------------------");
      localdata.applyEach(Printer());
#if defined USE_LOCAL
      sendLOCALMQTT();
#elif defined USE_MQTT
      sendThingspeakMQTT();
#elif defined USE_REST
      sendThingspeakREST();
#endif
    }
    else {
      // Parser error, print error
      Serial.println("---parse error-------------------------------------------------");
      Serial.println(err);
    } // parse result ok
  } // full telegram available
} // loop

/*****************************************************************************/

void sendLOCALMQTT() {
  while (!mqtt_client.connected()) {
    Serial.print("Reconnecting to MQTT...");
    if (mqtt_client.connect(CLIENTID)) {
      Serial.println(" done!");
    }
    else {
      Serial.println(" failed.");
      resetCounter++;
      if (resetCounter >= 5 )
        ESP.restart();
      delay(5000);
    }
  }

  if (mqtt_client.connected()) {
    digitalWrite(LED_BUILTIN, LOW);  // it is inverted
    lastLed = millis();

    mqtt_client.publish ("p1/power_delivered",          String(globaldata->power_delivered).c_str());
    mqtt_client.publish ("p1/power_returned",           String(globaldata->power_returned).c_str());
    mqtt_client.publish ("p1/gas_delivered",            String(globaldata->gas_delivered).c_str());
    mqtt_client.publish ("p1/energy_delivered_tariff1", String(globaldata->energy_delivered_tariff1).c_str());
    mqtt_client.publish ("p1/energy_delivered_tariff2", String(globaldata->energy_delivered_tariff2).c_str());
    mqtt_client.publish ("p1/energy_returned_tariff1",  String(globaldata->energy_returned_tariff1).c_str());
    mqtt_client.publish ("p1/energy_returned_tariff2",  String(globaldata->energy_returned_tariff2).c_str());
  }
} // sendLOCALMQTT

/*****************************************************************************/

void sendThingspeakMQTT() {
  while (!mqtt_client.connected()) {
    Serial.print("Reconnecting to MQTT...");
    if (mqtt_client.connect(CLIENTID, USERNAME, PASSWORD)) {
      Serial.println(" done!");
    }
    else {
      Serial.println(" failed.");
      resetCounter++;
      if (resetCounter >= 5 )
        ESP.restart();
      delay(5000);
    }
  }

  if (mqtt_client.connected()) {
    digitalWrite(LED_BUILTIN, LOW);  // it is inverted
    lastLed = millis();

    // Construct MQTT message
    String body = "field1=";
    body += String(globaldata->power_delivered);
    body += String("&field2=");
    body += String(globaldata->power_returned);
    body += String("&field3=");
    body += String(globaldata->gas_delivered);
    body += String("&field4=");
    body += String(globaldata->energy_delivered_tariff1);
    body += String("&field5=");
    body += String(globaldata->energy_delivered_tariff2);
    body += String("&field6=");
    body += String(globaldata->energy_returned_tariff1);
    body += String("&field7=");
    body += String(globaldata->energy_returned_tariff2);

    Serial.println("---mqtt message------------------------------------------------");
    Serial.println(body);
    if (mqtt_client.publish("channels/" CHANNEL "/publish", body.c_str()))
      resetCounter = 0;
  }
} // sendThingspeakMQTT

/*****************************************************************************/

void sendThingspeakREST() {
  if (wifi_client.connect(server, port)) {
    digitalWrite(LED_BUILTIN, LOW);  // it is inverted
    lastLed = millis();

    // Construct request body
    String body = "field1=";
    body += String(globaldata->power_delivered);
    body += String("&field2=");
    body += String(globaldata->power_returned);
    body += String("&field3=");
    body += String(globaldata->gas_delivered);
    body += String("&field4=");
    body += String(globaldata->energy_delivered_tariff1);
    body += String("&field5=");
    body += String(globaldata->energy_delivered_tariff2);
    body += String("&field6=");
    body += String(globaldata->energy_returned_tariff1);
    body += String("&field7=");
    body += String(globaldata->energy_returned_tariff2);

    Serial.println("---post url----------------------------------------------------");
    Serial.println(body);

    // Send message to thingspeak
    wifi_client.print("POST /update HTTP/1.1\n");
    wifi_client.print("Host: " + String(server) + "\n");
    wifi_client.print("Connection: close\n");
    wifi_client.print("X-THINGSPEAKAPIKEY: " + String(APIKEY) + "\n");
    wifi_client.print("Content-Type: application/x-www-form-urlencoded\n");
    wifi_client.print("Content-Length: " + String(body.length()) + "\n");
    wifi_client.print("\n");
    wifi_client.print(body);
    wifi_client.print("\n");

    Serial.println("---server response---------------------------------------------");
    while (wifi_client.available()) {
      char c = wifi_client.read();
      Serial.print(c);
    }

    wifi_client.stop();
    resetCounter = 0;
  }
  else {
    Serial.println("REST connection Failed.");
    Serial.println();

    resetCounter++;
    if (resetCounter >= 5 ) {
      ESP.restart();
    }
  }
} // sendThingspeakREST
