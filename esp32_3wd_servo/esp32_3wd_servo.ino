#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include <OSCMessage.h>   // https://github.com/CNMAT/OSC
#include <ESP32Servo.h>
#include <math.h>

WiFiUDP Udp;
OSCErrorCode error;
Servo servo1, servo2, servo3;

const char *host = "3WD-SERVO";
const char *version = __DATE__ " / " __TIME__;

const unsigned int inPort = 8000;   // local OSC port
const unsigned int outPort = 9000;  // remote OSC port (not used here)

// GPIO connections to the servo control
const int servo1_pin = 25;
const int servo2_pin = 26;
const int servo3_pin = 27;

// published values for the TS90D servo, neutral is 1500 uS
const int minUs = 500;
const int maxUs = 2500;

const float pr = 0.06;            // platform radius, distance from the platform center to each of the wheels, in meter
float vx = 0, vy = 0, vt = 0;     // speed in meter per second, and in radians per second
float x = 0, y = 0, theta = 0;    // absolute position (in meter) and rotation (in radians)
float r1 = 0, r2 = 0, r3 = 0;     // speed of the stepper motors, in steps per second

// don't use the built-in version of map(), as that is only for long int values
// see https://docs.arduino.cc/language-reference/en/functions/math/map/
#define map(value, fromLow, fromHigh, toLow, toHigh) ((toHigh - toLow) * (value - fromLow) / (fromHigh - fromLow) + toLow)

void printCallback(OSCMessage &msg) {
  Serial.print(msg.getAddress());
  Serial.print(" : ");

  for (int i = 0; i < msg.size(); i++) {
    if (msg.isInt(i)) {
      Serial.print(msg.getInt(i));
    } else if (msg.isFloat(i)) {
      Serial.print(msg.getFloat(i));
    } else if (msg.isDouble(i)) {
      Serial.print(msg.getDouble(i));
    } else if (msg.isBoolean(i)) {
      Serial.print(msg.getBoolean(i));
    } else if (msg.isString(i)) {
      char buffer[256];
      msg.getString(i, buffer);
      Serial.print(buffer);
    } else {
      Serial.print("?");
    }

    if (i < (msg.size() - 1)) {
      Serial.print(", ");  // there are more to come
    }
  }
  Serial.println();
}

void accxyzCallback(OSCMessage &msg) {
  vx = msg.getFloat(0);
  vy = msg.getFloat(1);
  vt = 0;
  updateSpeed();
}

void updateSpeed() {
  // convert the speed in world-coordinates into rotation speed of the motors and into servo controls
  // see https://github.com/manav20/3-wheel-omni

  // pr is the platform radius, i.e. the distance of the center of the platform to each wheel 
  // vx, vy, and vt is the speed in world-coordinates
  // r1, r2, and r3 is the rotation speed of the three motors
  // theta is the heading of the robot (FIXME, needs to be integrated over time)

  r1 = -sin(theta) * vx + cos(theta) * vy + pr * vt;
  r2 = -sin(PI / 3 - theta) * vx - cos(PI / 3 - theta) * vy + pr * vt;
  r3 = sin(PI / 3 + theta) * vx - cos(PI / 3 + theta) * vy + pr * vt;

  // convert the rotation speed of the motors into servo control values between 0 and 180, where 90 is neutral
  r1 = map(r1, -1, 1, 0, 180);
  r2 = map(r2, -1, 1, 0, 180);
  r3 = map(r3, -1, 1, 0, 180);

  servo1.write(r1);
  servo2.write(r2);
  servo3.write(r3);

  Serial.print(r1);
  Serial.print(", ");
  Serial.print(r2);
  Serial.print(", ");
  Serial.print(r3);
  Serial.println();
}

void wheel1Callback(OSCMessage &msg) {
  r1 = msg.getFloat(0);
  servo1.write(r1);
}

void wheel2Callback(OSCMessage &msg) {
  r2 = msg.getFloat(0);
  servo2.write(r2);
}

void wheel3Callback(OSCMessage &msg) {
  r3 = msg.getFloat(0);
  servo3.write(r3);
}

void setup() {
  Serial.begin(115200);

  Serial.print("[ ");
  Serial.print(host);
  Serial.print(" / ");
  Serial.print(version);
  Serial.println(" ]");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  WiFi.hostname(host);
  WiFi.begin();

  WiFiManager wifiManager;
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(host);
  Serial.println("connected");

  // Used for OSC
  Udp.begin(inPort);

  servo1.attach(servo1_pin, minUs, maxUs);
  servo2.attach(servo2_pin, minUs, maxUs);
  servo3.attach(servo3_pin, minUs, maxUs);
}

void loop() {
  OSCMessage msg;
  int size = Udp.parsePacket();

  if (size > 0) {
    while (size--) {
      msg.fill(Udp.read());
    }
    if (!msg.hasError()) {
      msg.dispatch("/accxyz", accxyzCallback);
      msg.dispatch("/wheel1", wheel1Callback);
      msg.dispatch("/wheel2", wheel2Callback);
      msg.dispatch("/wheel3", wheel3Callback);
      msg.dispatch("/*/*/*", printCallback) || msg.dispatch("/*/*", printCallback) || msg.dispatch("/*", printCallback);
    } else {
      error = msg.getError();
      Serial.print("error: ");
      Serial.println(error);
    }
  }
}
