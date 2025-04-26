/* 
This Arduino sketch is for a battery operated pet feeder based on a XIAO ESP32c6. It uses deep 
sleep to conserve power. When it wakes up, it gets a JSON document from a URL that specifies whether 
and how much to feed. It then moves a servo to provide the required number of portions and goes
back to sleep.

The JSON document is dynamically hosted by Node-Red running on a Raspberri Pi, which allows for
reprogramming the feeding schedule.

To prevent the servo motor from jerking to the default 0 angle upon every wake up and also to 
preserve power, it is connected over a bs170 transistor. This alows the ESP to switch the servo
motor power on only when needed.
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

#include "secret.h"

#define SERVO_CONTROL 20          // D9 is GPIO20, this is the control line for the servo
#define SERVO_ENABLE 18           // D10 is GPIO18, this goes over a 1 kOhm resistor to a BS170 transistor 
#define uS_TO_S_FACTOR 1000000ULL // conversion factor for micro seconds to seconds

WiFiClient wifi_client;
HTTPClient http_client;
Servo myservo;

// defaults following a hard reset
RTC_DATA_ATTR int interval = 60;
RTC_DATA_ATTR int amount = 0;

const long wifiTimeout = 10000;
const char* version = __DATE__ " / " __TIME__;

void deepsleep() {
  Serial.print("Going asleep for ");
  Serial.print(interval);
  Serial.println(" seconds.");
  Serial.flush();
  wifi_client.flush();
  wifi_client.stop();
  esp_sleep_enable_timer_wakeup(interval * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void moveServo() {
  // these have been experimentally calibrated
  const int left = 10;
  const int right = 74;
  const int middle = (left + right) / 2;

  Serial.println("Moving servo.");

  // start in the middle position
  myservo.write(middle);
  // switch on the power to the servo motor
  digitalWrite(SERVO_ENABLE, HIGH);

  // move to the right to pick up the food
  for (int posDegrees = middle; posDegrees <= right; posDegrees++) {
    myservo.write(posDegrees);
    delay(50);
  }

  // wait for it to drop into the slider
  delay(1000);

  // move to the left to drop the food
  for (int posDegrees = right; posDegrees >= left; posDegrees--) {
    myservo.write(posDegrees);
    delay(50);
  }

  // wait for it to drop out of the slider
  delay(1000);

  // move back to the middle position
  for (int posDegrees = left; posDegrees <= middle; posDegrees++) {
    myservo.write(posDegrees);
    delay(50);
  }

  // switch off the power to the servo motor
  digitalWrite(SERVO_ENABLE, LOW);
}

void setup() {
  Serial.begin(115200);
  Serial.print("\n[esp32c6_feeder / ");
  Serial.print(version);
  Serial.println("]");

  // switch on the LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);  // it is inverted, this switches it on

  // set up the servo, but initially with the power disabled
  pinMode(SERVO_ENABLE, OUTPUT);
  digitalWrite(SERVO_ENABLE, LOW);
  myservo.setPeriodHertz(50);
  myservo.attach(SERVO_CONTROL, 500, 2500);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  long now = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    if ((millis() - now) > wifiTimeout) {
      Serial.println("");
      Serial.println("failed!");
      deepsleep();  // try again later
    }
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());

  // get the instructions from a Raspberry Pi that is running Node-Red
  http_client.useHTTP10(true);
  http_client.begin("http://192.168.1.16:1880/feeder");
  if (http_client.GET()) {
    String payload = http_client.getString();
    Serial.println(payload);

    StaticJsonDocument<256> doc;
    deserializeJson(doc, payload);

    // these default to 0 when they are not present in the JSON document
    interval = doc["interval"].as<int>();
    amount = doc["amount"].as<int>();

    Serial.print("interval = ");
    Serial.println(interval);

    Serial.print("amount = ");
    Serial.println(amount);

    // provide food by moving the servo
    for (int i = 0; i < amount; i++)
      moveServo();
  }
  http_client.end();

  deepsleep();
}

void loop() {
  // it never gets here
}
