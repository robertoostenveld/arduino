#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <OSCMessage.h>  // https://github.com/CNMAT/OSC
#include <OSCBundle.h>
#include <ContinuousStepper.h>  // https://github.com/bblanchon/ArduinoContinuousStepper
#include <math.h>

#include "webinterface.h"
#include "waypoints.h"
#include "blink_led.h"

const char *host = "3WD-STEPPER";
const char *version = __DATE__ " / " __TIME__;

WebServer server(80);
WiFiUDP Udp;

enum Mode {
  PROG,
  USER
} mode;

ContinuousStepper<FourWireStepper> wheel1;
ContinuousStepper<FourWireStepper> wheel2;
ContinuousStepper<FourWireStepper> wheel3;

const unsigned int inPort = 8000;   // local OSC port for receiving commands
const float pd = 0.120;             // platform diameter, in meter
const float wd = 0.038;             // wheel diameter, in meter
const float pc = 0.3770;            // platform circumference, in meter
const float wc = 0.1197;            // wheel circumference, in meter
const float totalsteps = 2048;      // steps per revolution, see http://www.mjblythe.com/hacks/2016/09/28byj-48-stepper-motor/
const float maxsteps = 512;         // maximum steps per seconds, determined experimentally
const int direction = -1;           // rotation direction of the motors

unsigned long previous = 0;         // timer to integrate the speed over time
unsigned long feedback = 0;         // timer for feedback on the serial console
unsigned long offset = 4294967295;  // time at which the automatically executed program started

float vx = 0, vy = 0, vtheta = 0;   // speed in meter per second, and in radians per second
float x = 0, y = 0, theta = 0;      // absolute position (in meter) and rotation (in radians)
float r1 = 0, r2 = 0, r3 = 0;       // speed of the stepper motors, in steps per second

/********************************************************************************/

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

void progCallback(OSCMessage &msg) {
  // start the programmed movement
  mode = PROG;
  offset = millis();
  x = 0;
  y = 0;
  theta = 0;
  vx = 0;
  vy = 0;
  vtheta = 0;
}

void stopCallback(OSCMessage &msg) {
  // this will stop the movement and switch to user-mode
  mode = USER;
  offset = 4294967295;
  x = 0;
  y = 0;
  theta = 0;
  vx = 0;
  vy = 0;
  vtheta = 0;
}

void userCallback(OSCMessage &msg) {
  // this will stop the program but not the movement
  mode = USER;
  offset = 4294967295;
}

void xCallback(OSCMessage &msg) {
  vx = msg.getFloat(0);
}

void yCallback(OSCMessage &msg) {
  vy = msg.getFloat(0);
}

void thetaCallback(OSCMessage &msg) {
  vtheta = msg.getFloat(0);
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

/********************************************************************************/

void parseOSC() {
  int size = Udp.parsePacket();

  if (size > 0) {
    OSCMessage msg;
    OSCBundle bundle;
    OSCErrorCode error;

    char c = Udp.peek();
    if (c == '#') {
      // it is a bundle, these are sent by TouchDesigner
      while (size--) {
        bundle.fill(Udp.read());
      }
      if (!bundle.hasError()) {
        bundle.dispatch("/accxyz", accxyzCallback);  // this is for the accelerometer in touchOSC
        bundle.dispatch("/3wd/xy/1", xyCallback);    // this is for the 2D panel in touchOSC
        bundle.dispatch("/3wd/x", xCallback);
        bundle.dispatch("/3wd/y", yCallback);
        bundle.dispatch("/3wd/theta", thetaCallback);
        bundle.dispatch("/3wd/user", userCallback);
        bundle.dispatch("/3wd/prog", progCallback);
        bundle.dispatch("/3wd/stop", stopCallback);
      } else {
        error = bundle.getError();
        Serial.print("error: ");
        Serial.println(error);
      }
    } else if (c == '/') {
      // it is a message, these are sent by touchOSC
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
        msg.dispatch("/3wd/user", userCallback);
        msg.dispatch("/3wd/prog", progCallback);
        msg.dispatch("/3wd/stop", stopCallback);
      } else {
        error = msg.getError();
        Serial.print("error: ");
        Serial.println(error);
      }
    }
  }  // if size
}  // parseOSC

/********************************************************************************/

float maxOfThree(float a, float b, float c) {
  if (a >= b && a >= c) {
    return a;
  }
  else if (b >= a && b >= c) {
    return b;
  }
  else {
    return c;
  }
}

void updateSpeed() {
  // convert the speed from world-coordinates into the rotation speed of the motors
  r1 = sin(theta) * vx / wc - cos(theta) * vy / wc - vtheta * (pc / wc) / (PI * 2);
  r2 = sin(theta + PI * 2 / 3) * vx / wc - cos(theta + PI * 2 / 3) * vy / wc - vtheta * (pc / wc) / (PI * 2);
  r3 = sin(theta - PI * 2 / 3) * vx / wc - cos(theta - PI * 2 / 3) * vy / wc - vtheta * (pc / wc) / (PI * 2);

  // convert from rotations per second into steps per second
  r1 *= totalsteps * direction;
  r2 *= totalsteps * direction;
  r3 *= totalsteps * direction;

  // the stepper motors cannot rotate at more than ~512 steps/second
  float r = maxOfThree(abs(r1), abs(r2), abs(r3));
  if (r > maxsteps) {
    float reduction = r / maxsteps;
    Serial.print("reducing speed by factor ");
    Serial.println(reduction);
    r1 /= reduction;
    r2 /= reduction;
    r3 /= reduction;
    vx /= reduction;
    vy /= reduction;
    vtheta /= reduction;
  }

  if (r1 == 0 && r2 == 0 && r3 == 0) {
    // save energy by switching the motors off
    wheel1.powerOff();
    wheel2.powerOff();
    wheel3.powerOff();
    ledOn();
  }
  else {
    wheel1.powerOn();
    wheel2.powerOn();
    wheel3.powerOn();
    ledSlow();
  }

  // set the motor speed in steps per seconds
  wheel1.spin(r1);
  wheel2.spin(r2);
  wheel3.spin(r3);
}  // updateSpeed

/********************************************************************************/

void updatePosition() {
  unsigned long now = millis();

  // integrate the speed over time to keep track of the position
  x     += vx     * (now - previous) / 1000;
  y     += vy     * (now - previous) / 1000;
  theta += vtheta * (now - previous) / 1000;
  previous = now;

}  // updatePosition

/********************************************************************************/

void printPosition() {
  if (!config.serialfeedback)
    return;

  // do not print more than once every 500ms
  unsigned long now = millis();
  if ((now - feedback) > 500) {
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.print(", ");
    Serial.print(theta * 180 / M_PI); // in degrees
    Serial.print(", ");
    Serial.print(vx);
    Serial.print(", ");
    Serial.print(vy);
    Serial.print(", ");
    Serial.print(vtheta * 180 / M_PI); // in degrees per second
    Serial.print(", ");
    Serial.print(r1);
    Serial.print(", ");
    Serial.print(r2);
    Serial.print(", ");
    Serial.print(r3);
    Serial.print("\n");
    feedback = now;
  }
}  // printPosition

/********************************************************************************/

void updateProgram() {
  // this only applies when the program is running
  if (mode != PROG)
    return;

  // the N waypoints are connected by N-1 segments
  // determine on which segment we currently are
  unsigned long now = millis();
  int segment = -1;
  int num = waypoints_time.size();
  for (int i = 0; i < (num - 1); i++) {
    unsigned long segment_starttime = offset + 1000 * waypoints_time.at(i);
    unsigned long segment_endtime   = offset + 1000 * waypoints_time.at(i + 1);
    unsigned long program_endtime   = offset + 1000 * waypoints_time.at(num - 1);
    if (now >= segment_starttime && now < segment_endtime) {
      segment = i;
      break;
    }
    else if (now >= program_endtime && config.repeat) {
      // repeat the program
      segment = 0;
      offset = now;
      break;
    } // if
  } // for

  if (segment >= 0) {
    // determine the distance to travel along this segment, and the amount of time it should take
    float dt     = waypoints_time.at(segment + 1)  - waypoints_time.at(segment);
    float dx     = waypoints_x.at(segment + 1)     - waypoints_x.at(segment);
    float dy     = waypoints_y.at(segment + 1)     - waypoints_y.at(segment);
    float dtheta = waypoints_theta.at(segment + 1) - waypoints_theta.at(segment);
    // compute the speed
    vx = dx / dt;
    vy = dy / dt;
    vtheta = dtheta / dt;
  }
  else {
    vx = 0;
    vy = 0;
    vtheta = 0;
  }

} // updateProgram

/********************************************************************************/

void setup() {
  Serial.begin(115200);

  Serial.print("[ ");
  Serial.print(host);
  Serial.print(" / ");
  Serial.print(version);
  Serial.println(" ]");

  ledInit();
  ledFast();

  // The SPIFFS file system contains the html and javascript code for the web interface, and the "config.json" file
  if (!SPIFFS.begin()) {
    Serial.println("Error opening file system");
    while (true);
  }
  else {
    Serial.println("Opened file system");
    loadConfig();
    printConfig();
  }

  WiFi.hostname(host);
  WiFi.begin();

  WiFiManager wifiManager;
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(host);
  Serial.println("Connected to WiFi");

  // Used for OSC
  Udp.begin(inPort);

  // this serves all URIs that can be resolved to a file on the SPIFFS filesystem
  server.onNotFound(handleNotFound);

  server.on("/", HTTP_GET, []() {
    handleRedirect("/index.html");
  });

  server.on("/defaults", HTTP_GET, []() {
    Serial.println("handleDefaults");
    handleStaticFile("/reload_success.html");
    defaultConfig();
    printConfig();
    saveConfig();
    server.close();
    server.stop();
    ESP.restart();
  });

  server.on("/reconnect", HTTP_GET, []() {
    Serial.println("handleReconnect");
    handleStaticFile("/reload_success.html");
    server.close();
    server.stop();
    delay(5000);
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.startConfigPortal(host);
    server.begin();
    if (WiFi.status() == WL_CONNECTED)
      Serial.println("WiFi status ok");
  });

  server.on("/restart", HTTP_GET, []() {
    Serial.println("handleRestart");
    handleStaticFile("/reload_success.html");
    server.close();
    server.stop();
    SPIFFS.end();
    delay(5000);
    ESP.restart();
  });

  server.on("/json", HTTP_PUT, handleJSON);

  server.on("/json", HTTP_POST, handleJSON);

  server.on("/json", HTTP_GET, [] {
    JsonDocument root;
    N_CONFIG_TO_JSON(repeat, "repeat");
    N_CONFIG_TO_JSON(serialfeedback, "serialfeedback");
    N_CONFIG_TO_JSON(unused2, "unused2");
    root["version"] = version;
    root["uptime"] = long(millis() / 1000);
    root["waypoints"] = loadWaypoints();
    String str;
    serializeJson(root, str);
    server.setContentLength(str.length());
    server.send(200, "application/json", str);
  });

  // start the web server
  server.begin();

  // announce the hostname and web server through zeroconf
  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);

  // the pin activation sequence is IN1-IN3-IN2-IN4
  wheel1.begin(14, 26, 27, 25);
  wheel2.begin(13, 2, 15, 4);
  wheel3.begin(16, 5, 17, 18);

  // The SPIFFS file system contains the "waypoints.csv" file for predefined movements
  parseWaypoints();

  // start in user mode
  mode = USER;
}  // setup

/********************************************************************************/

void loop() {
  updatePosition();       // integrate the velocity to get an estimate of the position and heading
  printPosition();        // print the current position and speed
  updateProgram();        // update the speed according to the programmed waypoints
  updateSpeed();          // translate the world speed into motor speeds

  // keep the wheels moving at the proper speed
  wheel1.loop();
  wheel2.loop();
  wheel3.loop();

  parseOSC();             // parse the incoming OSC messages
  server.handleClient();  // handle webserver requests, note that this may disrupt other processes
}  // loop
