/*
  This Arduino sketch is for an ESP32-based automated pet feeder system that operates 
  on a sleep-wake cycle to conserve power. After performing its tasks, the ESP32 goes 
  back to deep sleep before waking up again.

  The main features are:
  - Low power consumption by using deep sleep.
  - Persistent memory to keep track of wakeups, last clock sync, and last feeding time.
  - Logging to report events to an MQTT server for remote monitoring.
  - Ensures food is dispensed only once per day and not more than once per hour.
  - Servo motor control.

*/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>  // https://github.com/knolleary/pubsubclient
#include <ESP32Servo.h>

#include "secret.h"  // contains wifi and mqtt credentials
#include "util.h"    // contains some helper functions

#define SHORT_SLEEP 60  // Time ESP32 will go to sleep when wifi fails (in seconds)
#define LONG_SLEEP 600  // Time ESP32 will go to sleep between wakeup (in seconds)

const char *host = "ESP32C6-FEEDER";
const char *version = __DATE__ " / " __TIME__;

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);
Servo myservo;

long now = 0;
struct tm timeinfo;  // this has multiple fields, nicely organized
struct timeval tv;   // this has tv_sec and tv_usec

bool updateClock = 0;
bool provideFood = 0;

const long setupTimeout = 10000;          // in milliseconds, when the setup fails
const unsigned int clockTimeout = 86400;  // in seconds, update the clock once per day

// these variables survive deep sleep
RTC_DATA_ATTR unsigned long wakeupCount = 0;
RTC_DATA_ATTR struct timeval lastClockUpdate;
RTC_DATA_ATTR int feedingHour = 14;   // in hours of the day, 0-23
RTC_DATA_ATTR int feedingMinute = 0;  // in minutes, 0-59
RTC_DATA_ATTR int feedingAmount = 1;  // how many portions to serve

void setup() {
  Serial.begin(115200);
  Serial.println("------------------------------------------------------------");
  Serial.print("[ ");
  Serial.print(host);
  Serial.print(" / ");
  Serial.print(version);
  Serial.println(" ]");
  Serial.println("Starting setup");

  // set up the servo, but with the power disabled
  pinMode(SERVO_ENABLE, OUTPUT);
  digitalWrite(SERVO_ENABLE, LOW);
  myservo.setPeriodHertz(50);
  myservo.attach(SERVO_CONTROL, 500, 2500);

  // Blink slowly for two seconds
  pinMode(LED_BUILTIN, OUTPUT);
  now = millis();
  while ((millis() - now) < 2000) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(500);
  }

  // Set the timezone to Europe/Amsterdam, with DST
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  printWakeupReason();
  Serial.println("Wakeup count: " + String(++wakeupCount));

  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
    case ESP_SLEEP_WAKEUP_EXT1:
    case ESP_SLEEP_WAKEUP_TIMER:
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
    case ESP_SLEEP_WAKEUP_ULP:
      getLocalTime(&timeinfo);  // this has multiple fields, nicely organized
      gettimeofday(&tv, NULL);  // this has tv_sec and tv_usec, easier for time comparisons
      if (timeinfo.tm_hour == feedingHour && timeinfo.tm_min == feedingMinute) {
        updateClock = 0;
        provideFood = 1;
      } else if (timeDifference(tv, lastClockUpdate) > clockTimeout) {
        updateClock = 1;
        provideFood = 0;
      } else {
        updateClock = 0;
        provideFood = 0;
      }
      break;
    default:
      // this is upon a hard reset
      updateClock = 1;
      provideFood = 0;
      lastClockUpdate.tv_sec = 0;
      lastClockUpdate.tv_usec = 0;
      lastFood.tv_sec = 0;
      lastFood.tv_usec = 0;
      break;
  }

  printLocalTime1();
  // printLocalTime2();

  // provide feedback on the actions that will be performed
  Serial.print("updateClock = ");
  Serial.println(updateClock);
  Serial.print("provideFood = ");
  Serial.println(provideFood);

  Serial.println("Connecting to WiFi...");
  WiFi.persistent(false);
  WiFi.begin(ssid, password);
  now = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    if ((millis() - now) > setupTimeout) {
      Serial.println("");
      Serial.println("failed!");
      enterDeepSleep(SHORT_SLEEP);  // and try again later
    }
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());

  Serial.print("Connecting to MQTT...");
  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  now = millis();
  while (!mqtt_client.connect(mqtt_clientid, mqtt_username, mqtt_password)) {
    Serial.print(".");
    if ((millis() - now) > setupTimeout) {
      Serial.println("");
      Serial.println("failed!");
      enterDeepSleep(SHORT_SLEEP);  // and try again later
    }
    delay(500);
  }
  Serial.println("");
  Serial.println("MQTT connected");
  mqtt_client.subscribe("feeder/time");
  mqtt_client.subscribe("feeder/amount");

  if (updateClock) {
    Serial.print("Connecting to NTP server...");
    setenv("TZ", "UTC", 1);
    tzset();
    configTime(0, 0, "nl.pool.ntp.org");  // get the time in UTC
    long now = millis();
    while (!getLocalTime(&timeinfo)) {
      Serial.print(".");
      if ((millis() - now) > setupTimeout) {
        Serial.println("");
        Serial.println("failed!");
        enterDeepSleep(SHORT_SLEEP);  // and try again later
      }
      delay(500);
    }
    gettimeofday(&tv, NULL);
    lastClockUpdate.tv_sec = tv.tv_sec;
    lastClockUpdate.tv_usec = tv.tv_usec;
    Serial.println("");
    Serial.println("NTP setup done");
  }

  // set the timezone to Europe/Amsterdam, with DST
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  printLocalTime1();
  //printLocalTime2();

  sendMessage("feeder/check");

  if (provideFood) {
    sendMessage("feeder/food");

    // move the servo so that some food falls out
    for (int i = 0; i < feedingAmount; i++)
      moveServo();
  }

  // some delay is needed to send the MQTT messages, blink rapidly
  long now = millis();
  while ((millis() - now) < 2000) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    mqtt_client.loop();
    delay(100);
  }

  enterDeepSleep(LONG_SLEEP);
}

void loop() {
  // this part is never reached
}
