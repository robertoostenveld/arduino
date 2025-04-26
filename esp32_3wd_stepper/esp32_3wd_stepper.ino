#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <math.h>

#include "webinterface.h"
#include "waypoints.h"
#include "parseosc.h"
#include "stepper.h"
#include "blink_led.h"
#include "util.h"

const char *host = "3WD-STEPPER";
const char *version = __DATE__ " / " __TIME__;

WebServer server(80);
WiFiUDP Udp;

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

float x = 0, y = 0, a = 0;          // absolute position (in meter) and angle (in radians)
float vx = 0, vy = 0, va = 0;       // speed in meter per second, and angular speed in radians per second
float r1 = 0, r2 = 0, r3 = 0;       // speed of the stepper motors, in steps per second

float target_x = 0, target_y = 0, target_a = 0;
unsigned long route_starttime = 4294967295;
unsigned long target_eta = 4294967295;

// when there is an active route, the speed is determined by the distance to the target waypoint
// when there is no route, the speed is determined manually by the user
int current_route = -1;
int current_segment = -1;
bool route_pause = false;

/********************************************************************************/

void startRoute(int route) {
  unsigned long now = millis();

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

  // insert the current position and orientation as the starting point
  waypoints_t.insert(waypoints_t.begin(), 0);
  waypoints_x.insert(waypoints_x.begin(), x);
  waypoints_y.insert(waypoints_y.begin(), y);
  waypoints_a.insert(waypoints_a.begin(), a);

  // start the new route
  route_starttime = now;
  current_route = route;
  route_pause = false;

  if (config.debug)
    printWaypoints(route);
} // startRoute

/********************************************************************************/

void pauseRoute() {
  // pause the current route
  vx = 0;
  vy = 0;
  va = 0;
  route_pause = true;
} // pauseRoute

/********************************************************************************/

void resumeRoute() {
  // resume the current route
  route_pause = false;
} // resumeRoute

/********************************************************************************/

void stopRoute() {
  // stop the current route
  vx = 0;
  vy = 0;
  va = 0;
  current_route = -1;
  route_starttime = 4294967295;
  target_eta = 4294967295;
  route_pause = false;
} // stopRoute

/********************************************************************************/

void resetRoute() {
  // stop the current route and reset the current position to zero
  stopRoute();
  x = 0;
  y = 0;
  a = 0;
} // resetRoute

/********************************************************************************/

void updatePosition() {
  unsigned long now = millis();
  if (route_pause) {
    // shift the time so that we remain in the same state
    // this is as if the time has stopped
    route_starttime += (now - previous);
    target_eta += (now - previous);
  }
  else {
    // integrate the speed over time to keep track of the current position and angle
    x += vx * (now - previous) / 1000;
    y += vy * (now - previous) / 1000;
    a += va * (now - previous) / 1000;
  }
  previous = now;
}  // updatePosition

/********************************************************************************/

void updateTarget() {
  unsigned long now = millis();

  if (current_route < 0) {
    // when there is no route, the speed is determined manually by the user
    // hence the current position is as good a target as anywhere else
    target_x = x;
    target_y = y;
    target_a = a;
    target_eta = 4294967295;
  }
  else {
    // when there is an active route, the speed is determined by the distance to the target waypoint
    // determine the segment or leg that we are currently on
    current_segment = -1;

    int n = waypoints_t.size();
    unsigned long route_endtime = route_starttime + 1000 * waypoints_t.at(n - 1);

    if (now < route_starttime) {
      // the route has not yet started
      // stay where we are
      target_x = x;
      target_y = y;
      target_a = a;
      target_eta = 4294967295;
    }
    else if (now >= route_endtime) {
      // the route has finished
      if (config.repeat) {
        // start the same route again
        startRoute(current_route);
      }
      else {
        // stop the route
        current_route = -1;
        vx = 0;
        vy = 0;
        va = 0;
        // stay where we are
        target_x = x;
        target_y = y;
        target_a = a;
        target_eta = 4294967295;
      }
    }
    else {
      // determine the segment or leg that we are currently on
      // the N waypoints are connected by N-1 segments
      for (int i = 0; i < (n - 1); i++) {
        unsigned long segment_starttime = route_starttime + 1000 * waypoints_t.at(i);
        unsigned long segment_endtime   = route_starttime + 1000 * waypoints_t.at(i + 1);
        if (now >= segment_starttime && now < segment_endtime) {
          current_segment = i + 1;
          target_x = waypoints_x.at(i + 1);
          target_y = waypoints_y.at(i + 1);
          target_a = waypoints_a.at(i + 1);
          target_eta = segment_endtime;
          break;
        }
      } // for all segments
    } // if route_starttime
  } // if current_route

} // updateTarget

/********************************************************************************/

void updateSpeed() {
  unsigned long now = millis();

  if (current_route < 0) {
    // when there is no route, the speed is determined manually by the user
  }
  else if (route_pause) {
    // don't move if the route is paused
    vx = 0;
    vy = 0;
    va = 0;
  }
  else {
    // when there is an active route, the speed is determined by the distance to the target waypoint
    if (current_segment > 0) {
      // determine the speed needed to reach the target at the desired time
      float dt = 0.001 * (target_eta - now); // in seconds
      if (dt > 0) {
        vx = (target_x - x) / dt;
        vy = (target_y - y) / dt;
        va = (target_a - a) / dt;
      }
      else {
        // infinite or negative speed is not allowed
        vx = 0;
        vy = 0;
        va = 0;
      }
    }
    else {
      // the current segment could not be determined
      vx = 0;
      vy = 0;
      va = 0;
    } // if current_segment
  } // if current_route

} // updateSpeed

/********************************************************************************/

void updateWheels() {
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
    vx /= reduction;
    vy /= reduction;
    va /= reduction;
    r1 /= reduction;
    r2 /= reduction;
    r3 /= reduction;
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
}  // updateWheels

/********************************************************************************/

void printDebug() {
  unsigned long now = millis();

  // do not print more often than once every 500ms
  if ((now - feedback) > 500) {

    Serial.print("current = ");
    Serial.print(current_route);
    Serial.print(", ");
    Serial.print(current_segment);

    Serial.print(", time = ");
    Serial.print(now);
    Serial.print(", ");
    Serial.print(target_eta);

    Serial.print(", position = ");
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.print(", ");
    Serial.print(a * 180 / M_PI); // in degrees

    Serial.print(", target = ");
    Serial.print(target_x);
    Serial.print(", ");
    Serial.print(target_y);
    Serial.print(", ");
    Serial.print(target_a * 180 / M_PI); // in degrees

    Serial.print(", speed = ");
    Serial.print(vx);
    Serial.print(", ");
    Serial.print(vy);
    Serial.print(", ");
    Serial.print(va * 180 / M_PI); // in degrees per second

    Serial.print(", wheel speed = ");
    Serial.print(r1);
    Serial.print(", ");
    Serial.print(r2);
    Serial.print(", ");
    Serial.print(r3);

    Serial.println();
    feedback = now;
  }
}
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

  // incoming port for OSC messages
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
    N_CONFIG_TO_JSON(debug, "debug");
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

  Serial.println("Setup done");
} // setup

/********************************************************************************/

void loop() {
  updatePosition();       // update the current location
  updateTarget();         // update the target location
  updateSpeed();          // update the world speed
  updateWheels();         // update the speed of the wheels

  if (config.debug)
    printDebug();

  parseOSC();             // parse the incoming OSC messages
  server.handleClient();  // handle webserver requests, note that this may disrupt other processes
} // loop
