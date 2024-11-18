#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include <OSCMessage.h>         // https://github.com/CNMAT/OSC
#include <OSCBundle.h>
#include <ContinuousStepper.h>  // https://github.com/bblanchon/ArduinoContinuousStepper
#include <math.h>
#include <FS.h>
#include <SPIFFS.h>

const char *host = "3WD-STEPPER";
const char *version = __DATE__ " / " __TIME__;

WiFiUDP Udp;
OSCErrorCode error;

ContinuousStepper<FourWireStepper> wheel1;
ContinuousStepper<FourWireStepper> wheel2;
ContinuousStepper<FourWireStepper> wheel3;

const unsigned int inPort = 8000;   // local OSC port
const unsigned int outPort = 9000;  // remote OSC port (not needed for receive)

const float pd = 0.120;         // platform diameter, in meter
const float wd = 0.038;         // wheel diameter, in meter
const float pc = 0.3770;        // platform circumference, in meter
const float wc = 0.1197;        // wheel circumference, in meter
const float totalsteps = 2048;  // steps per revolution, see http://www.mjblythe.com/hacks/2016/09/28byj-48-stepper-motor/
const float maxsteps = 512;     // maximum steps per seconds

long previous = 0, feedback = 0;  // keep track of time
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

void disableCallback(OSCMessage &msg) {
  wheel1.powerOff();
  wheel2.powerOff();
  wheel3.powerOff();
}

void enableCallback(OSCMessage &msg) {
  wheel1.powerOn();
  wheel2.powerOn();
  wheel3.powerOn();
}

void resetCallback(OSCMessage &msg) {
  vx = 0;
  vy = 0;
  vt = 0;
  x = 0;
  y = 0;
  theta = 0;
}

void xCallback(OSCMessage &msg) {
  vx = msg.getFloat(0);
}

void yCallback(OSCMessage &msg) {
  vy = msg.getFloat(0);
}

void thetaCallback(OSCMessage &msg) {
  vt = msg.getFloat(0);
}

void xyCallback(OSCMessage &msg) {
  vx = msg.getFloat(0);
  vy = msg.getFloat(1);
}

void accxyzCallback(OSCMessage &msg) {
  // values are between -1 and 1, scale them to a more appropriate speed
  vx = msg.getFloat(0) * 0.03;
  vy = msg.getFloat(1) * 0.03;
}

void updateSpeed() {
  // convert the speed from world-coordinates into the rotation speed of the motors
  r1 = sin(theta) * vx / wc - cos(theta) * vy / wc - vt * (pc / wc) / (PI * 2);
  r2 = sin(theta + PI * 2 / 3) * vx / wc - cos(theta + PI * 2 / 3) * vy / wc - vt * (pc / wc) / (PI * 2);
  r3 = sin(theta - PI * 2 / 3) * vx / wc - cos(theta - PI * 2 / 3) * vy / wc - vt * (pc / wc) / (PI * 2);

  // convert from rotations per second into steps per second
  r1 = r1 * totalsteps;
  r2 = r2 * totalsteps;
  r3 = r3 * totalsteps;

  // the stepper motors cannot rotate at more than 512 steps/second
  if (abs(r1) > maxsteps) {
    float reduction = abs(r1) / maxsteps;
    Serial.print("reducing speed by factor ");
    Serial.println(reduction);
    r1 /= reduction;
    r2 /= reduction;
    r3 /= reduction;
    vx /= reduction;
    vy /= reduction;
    vt /= reduction;
  }

  // the stepper motors cannot rotate at more than 512 steps/second
  if (abs(r2) > maxsteps) {
    float reduction = abs(r2) / maxsteps;
    Serial.print("reducing speed by factor ");
    Serial.println(reduction);
    r1 /= reduction;
    r2 /= reduction;
    r3 /= reduction;
    vx /= reduction;
    vy /= reduction;
    vt /= reduction;
  }

  // the stepper motors cannot rotate at more than 512 steps/second
  if (abs(r3) > maxsteps) {
    float reduction = abs(r3) / maxsteps;
    Serial.print("reducing speed by factor ");
    Serial.println(reduction);
    r1 /= reduction;
    r2 /= reduction;
    r3 /= reduction;
    vx /= reduction;
    vy /= reduction;
    vt /= reduction;
  }

  // set the stepper motor speed
  wheel1.spin(r1);
  wheel2.spin(r2);
  wheel3.spin(r3);
}

void updatePosition() {
  long now = millis();

  // integrate the speed over time to keep track of the position
  x += vx * (now - previous) / 1000;
  y += vy * (now - previous) / 1000;
  theta += vt * (now - previous) / 1000;
  previous = now;

  if ((now - feedback) > 500) {
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.print(", ");
    Serial.print(theta);
    Serial.print(", ");
    Serial.print(vx);
    Serial.print(", ");
    Serial.print(vy);
    Serial.print(", ");
    Serial.print(vt);
    Serial.print(", ");
    Serial.print(r1);
    Serial.print(", ");
    Serial.print(r2);
    Serial.print(", ");
    Serial.print(r3);
    Serial.print("\n");
    feedback = now;
  }
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

  // For the default Arduino Stepper the pins are entered in the sequence IN1-IN3-IN2-IN4
  // For ContinuousStepper the pins are entered in the sequence IN1-IN2-IN3-IN4
  wheel1.begin(25, 27, 26, 14);
  wheel2.begin(22, 23, 19, 18);
  wheel3.begin(17, 16, 5, 4);
}

void loop() {
  updatePosition();
  updateSpeed();

  wheel1.loop();
  wheel2.loop();
  wheel3.loop();

  int size = Udp.parsePacket();
  if (size > 0) {

    char c = Udp.peek();
    if (c == '#') {
      // it is a bundle, these are sent by TouchDesigner
      OSCBundle bundle;

      while (size--) {
        bundle.fill(Udp.read());
      }
      if (!bundle.hasError()) {
        bundle.dispatch("/accxyz", accxyzCallback);  // this is for the accelerometer in touchOSC
        bundle.dispatch("/3wd/xy/1", xyCallback);    // this is for the 2D panel in touchOSC
        bundle.dispatch("/3wd/x", xCallback);
        bundle.dispatch("/3wd/y", yCallback);
        bundle.dispatch("/3wd/theta", thetaCallback);
        bundle.dispatch("/3wd/disable", disableCallback);
        bundle.dispatch("/3wd/enable", enableCallback);
        bundle.dispatch("/3wd/reset", resetCallback);
      } else {
        error = bundle.getError();
        Serial.print("error: ");
        Serial.println(error);
      }
    } else if (c == '/') {
      // it is a message, these are sent by touchOSC
      OSCMessage msg;
      
      while (size--) {
        msg.fill(Udp.read());
      }
      if (!msg.hasError()) {
        msg.dispatch("/*", printCallback) || msg.dispatch("/*/*", printCallback) || msg.dispatch("/*/*/*", printCallback);
        msg.dispatch("/accxyz", accxyzCallback);  // this is for the accelerometer in touchOSC
        msg.dispatch("/3wd/xy/1", xyCallback);    // this is for the 2D panel in touchOSC
        msg.dispatch("/3wd/x", xCallback);
        msg.dispatch("/3wd/y", yCallback);
        msg.dispatch("/3wd/theta", thetaCallback);
        msg.dispatch("/3wd/disable", disableCallback);
        msg.dispatch("/3wd/enable", enableCallback);
        msg.dispatch("/3wd/reset", resetCallback);
      } else {
        error = msg.getError();
        Serial.print("error: ");
        Serial.println(error);
      }
    }
  } // if size
} // loop
