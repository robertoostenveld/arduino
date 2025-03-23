#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <OSCMessage.h>         // https://github.com/CNMAT/OSC
#include <OSCBundle.h>
#include <math.h>

#include "webinterface.h"
#include "waypoints.h"
#include "stepper.h"
#include "blink_led.h"
#include "util.h"

const char *host = "3WD-STEPPER";
const char *version = __DATE__ " / " __TIME__;

WebServer server(80);
WiFiUDP Udp;

enum Mode {
  ROUTE,
  USER
} mode;

Stepper wheel1;
Stepper wheel2;
Stepper wheel3;

const unsigned int inPort = 8000;   // local OSC port for receiving commands
const float pd = 0.084252;          // platform diameter, in meter
const float wd = 0.038;             // wheel diameter, in meter
const float pc = pd * M_PI;         // platform circumference, in meter
const float wc = wd * M_PI;         // wheel circumference, in meter
const float totalsteps = 2048;      // steps per revolution, see http://www.mjblythe.com/hacks/2016/09/28byj-48-stepper-motor/
const float maxsteps = 512;         // maximum steps per seconds, determined experimentally

unsigned long previous = 0;         // timer to integrate the speed over time
unsigned long feedback = 0;         // timer for feedback on the serial console
unsigned long offset = 4294967295;  // time at which the route along the waypoints started

float x = 0, y = 0, a = 0;          // absolute position (in meter) and angle (in radians)
float vx = 0, vy = 0, va = 0;       // speed in meter per second, and angular speed in radians per second
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

void startRoute(int route) {
  // stop the current movement
  vx = 0;
  vy = 0;
  va = 0;
  if (!config.absolute) {
    // set the current position and orientation as zero
    x = 0;
    y = 0;
    a = 0;
  }
  // read the waypoints from the SPIFFS filesystem
  parseWaypoints(route);
  // insert the current position and orientation as the starting position
  waypoints_t.insert(waypoints_t.begin(), 0);
  waypoints_x.insert(waypoints_x.begin(), x);
  waypoints_y.insert(waypoints_y.begin(), y);
  waypoints_a.insert(waypoints_a.begin(), a);
  // start the new route
  printWaypoints(route);
  offset = millis();
  mode = ROUTE;
}

void route1Callback(OSCMessage &msg) {
  startRoute(1);
}

void route2Callback(OSCMessage &msg) {
  startRoute(2);
}

void route3Callback(OSCMessage &msg) {
  startRoute(3);
}

void route4Callback(OSCMessage &msg) {
  startRoute(4);
}

void route5Callback(OSCMessage &msg) {
  startRoute(5);
}

void route6Callback(OSCMessage &msg) {
  startRoute(6);
}

void route7Callback(OSCMessage &msg) {
  startRoute(7);
}

void route8Callback(OSCMessage &msg) {
  startRoute(8);
}

void stopCallback(OSCMessage &msg) {
  // stop the current movement
  vx = 0;
  vy = 0;
  va = 0;
  // switch to user-mode
  offset = 4294967295;
  mode = USER;
}

void resetCallback(OSCMessage &msg) {
  // stop the current movement
  vx = 0;
  vy = 0;
  va = 0;
  // reset the position
  x = 0;
  y = 0;
  a = 0;
  // switch to user-mode
  offset = 4294967295;
  mode = USER;
}

void xCallback(OSCMessage &msg) {
  vx = msg.getFloat(0);
}

void yCallback(OSCMessage &msg) {
  vy = msg.getFloat(0);
}

void thetaCallback(OSCMessage &msg) {
  va = msg.getFloat(0);
}

void xyCallback(OSCMessage &msg) {
  vx = msg.getFloat(0);
  vy = msg.getFloat(1);
}

void accxyzCallback(OSCMessage &msg) {
  // values are between -1 and 1, scale them to a more appropriate speed of 3 cm/s
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
        bundle.dispatch("/accxyz",     accxyzCallback);  // this is for the accelerometer in touchOSC
        bundle.dispatch("/3wd/xy/1",   xyCallback);      // this is for the 2D panel in touchOSC
        bundle.dispatch("/3wd/x",      xCallback);
        bundle.dispatch("/3wd/y",      yCallback);
        bundle.dispatch("/3wd/theta",  thetaCallback);
        bundle.dispatch("/3wd/route1", route1Callback);
        bundle.dispatch("/3wd/route2", route2Callback);
        bundle.dispatch("/3wd/route3", route3Callback);
        bundle.dispatch("/3wd/route4", route4Callback);
        bundle.dispatch("/3wd/route5", route5Callback);
        bundle.dispatch("/3wd/route6", route6Callback);
        bundle.dispatch("/3wd/route7", route7Callback);
        bundle.dispatch("/3wd/route8", route8Callback);
        bundle.dispatch("/3wd/stop",   stopCallback);
        bundle.dispatch("/3wd/reset",  resetCallback);
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
        msg.dispatch("/accxyz",     accxyzCallback);  // this is for the accelerometer in touchOSC
        msg.dispatch("/3wd/xy/1",   xyCallback);      // this is for the 2D panel in touchOSC
        msg.dispatch("/3wd/x",      xCallback);
        msg.dispatch("/3wd/y",      yCallback);
        msg.dispatch("/3wd/theta",  thetaCallback);
        msg.dispatch("/3wd/route1", route1Callback);
        msg.dispatch("/3wd/route2", route2Callback);
        msg.dispatch("/3wd/route3", route3Callback);
        msg.dispatch("/3wd/route4", route4Callback);
        msg.dispatch("/3wd/route5", route5Callback);
        msg.dispatch("/3wd/route6", route6Callback);
        msg.dispatch("/3wd/route7", route7Callback);
        msg.dispatch("/3wd/route8", route8Callback);
        msg.dispatch("/3wd/stop",   stopCallback);
        msg.dispatch("/3wd/reset",  resetCallback);
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

/********************************************************************************/

void updateSpeed() {
  // convert the speed from world-coordinates into the rotation speed of the motors
  r1 = sin(a               ) * vx / wc - cos(a               ) * vy / wc - va * (pc / wc) / (M_PI * 2);
  r2 = sin(a + M_PI * 2 / 3) * vx / wc - cos(a + M_PI * 2 / 3) * vy / wc - va * (pc / wc) / (M_PI * 2);
  r3 = sin(a - M_PI * 2 / 3) * vx / wc - cos(a - M_PI * 2 / 3) * vy / wc - va * (pc / wc) / (M_PI * 2);

  // convert from rotations per second into steps per second
  r1 *= totalsteps;
  r2 *= totalsteps;
  r3 *= totalsteps;

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
    va /= reduction;
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

  // integrate the speed over time to keep track of the position and angle
  x += vx * (now - previous) / 1000;
  y += vy * (now - previous) / 1000;
  a += va * (now - previous) / 1000;
  previous = now;

}  // updatePosition

/********************************************************************************/

void printPosition() {
  if (!config.serial)
    return;

  // do not print more than once every 500ms
  unsigned long now = millis();
  if ((now - feedback) > 500) {
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.print(", ");
    Serial.print(a * 180 / M_PI); // in degrees
    Serial.print(", ");
    Serial.print(vx);
    Serial.print(", ");
    Serial.print(vy);
    Serial.print(", ");
    Serial.print(va * 180 / M_PI); // in degrees per second
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

void updateRoute() {
  // this only applies when the route along the waypoints is being executed
  if (mode != ROUTE)
    return;

  // the N waypoints are connected by N-1 segments
  // determine on which segment we currently are
  unsigned long now = millis();
  int segment = -1;
  int n = waypoints_t.size();
  for (int i = 0; i < (n - 1); i++) {
    unsigned long segment_starttime = offset + 1000 * waypoints_t.at(i);
    unsigned long segment_endtime   = offset + 1000 * waypoints_t.at(i + 1);
    unsigned long route_endtime     = offset + 1000 * waypoints_t.at(n - 1);
    if (now >= segment_starttime && now < segment_endtime) {
      segment = i;
      break;
    }
    else if (now >= route_endtime && config.repeat) {
      // repeat the route along the waypoints
      Serial.println("repeat route");
      segment = 0;
      offset = now;
      break;
    } // if
  } // for

  if (segment >= 0) {
    // determine the distance along this segment, and the amount of time it should take
    float dx = waypoints_x.at(segment + 1) - waypoints_x.at(segment);
    float dy = waypoints_y.at(segment + 1) - waypoints_y.at(segment);
    float da = waypoints_a.at(segment + 1) - waypoints_a.at(segment);
    float dt = waypoints_t.at(segment + 1) - waypoints_t.at(segment);
    // compute the speed
    vx = dx / dt;
    vy = dy / dt;
    va = da / dt;
  }
  else {
    vx = 0;
    vy = 0;
    va = 0;
  }
} // updateRoute

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

  printMacAddress();

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
    N_CONFIG_TO_JSON(absolute, "absolute");
    N_CONFIG_TO_JSON(warp, "warp");
    N_CONFIG_TO_JSON(serial, "serial");
    root["version"] = version;
    root["uptime"] = long(millis() / 1000);
    root["macaddress"] = getMacAddress();
    root["waypoints1"] = loadWaypoints(1);
    root["waypoints2"] = loadWaypoints(2);
    root["waypoints3"] = loadWaypoints(3);
    root["waypoints4"] = loadWaypoints(4);
    root["waypoints5"] = loadWaypoints(5);
    root["waypoints6"] = loadWaypoints(6);
    root["waypoints7"] = loadWaypoints(7);
    root["waypoints8"] = loadWaypoints(8);
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

  // connect the timer to the stepper motor driver pins
  wheel1.begin(14, 27, 26, 25);
  wheel2.begin(13, 15, 2, 4);
  wheel3.begin(16, 17, 5, 18);

  // start in user mode
  mode = USER;
} // setup

/********************************************************************************/

void loop() {
  updatePosition();       // integrate the velocity to get an estimate of the position and heading
  printPosition();        // print the current position and speed
  updateRoute();          // update the speed of the wheels according to route along the waypoints
  updateSpeed();          // translate the world speed into motor speeds

  parseOSC();             // parse the incoming OSC messages
  server.handleClient();  // handle webserver requests, note that this may disrupt other processes
} // loop
