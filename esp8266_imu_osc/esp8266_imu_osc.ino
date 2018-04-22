/*
  This sketch is for an ESP8266 connected to an TCA9548A I2C multiplexer,
  which in turn connects to multiple MPU9250 inertial motion units (IMUs).

  It reads the acceletometer, gyroscope and magnetometer data from the
  IMUs and computes AHRS (yaw/pitch/roll) parameters. All data is combined
  and sent over UDP using the OSC protocol.

  It uses WIFiManager for initial configuration and includes a web-interface
  that allows to monitor and change parameters.

  The status of the wifi connection, http server and the IMU data transmission
  is indicated by a RDB led that blinks Red, Green or Blue.

*/

#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>         // https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <WiFiUDP.h>
#include <OSCBundle.h>
#include <FS.h>

#include "tca9548a.h"
#include "mpu9250.h"
#include "ahrs.h"
#include "setup_ota.h"
#include "rgb_led.h"

#define Debug true          // this is for Serial, but also needed for the LED to blink
#define SerialDebug false   // this is only desired for Serial debugging
#define maxSensors 8

// this allows some sections of the code to be disabled for debugging purposes
#define ENABLE_WEBINTERFACE
#define ENABLE_MDNS
#define ENABLE_IMU

Config config;
ESP8266WebServer server(80);
const char* host = "IMU-OSC";
const char* version = __DATE__ " / " __TIME__;

tca9548a tca;
mpu9250 mpu[maxSensors];
ahrs ahrs[maxSensors];
String id[maxSensors] = {"imu0", "imu1", "imu2", "imu3", "imu4", "imu5", "imu6", "imu7"};

// UDP destination address
IPAddress outIp(192, 168, 1, 144);
const unsigned int inPort = 9000, outPort = 8000;
WiFiUDP Udp;

float deltat = 0.0f, sum = 0.0f;          // integration interval for both filter schemes
float rate = 0.0f;
uint32_t Now = 0, Last[maxSensors];       // used to calculate integration interval
uint32_t lastDisplay = 0, lastTemperature = 0, lastTransmit = 0;
unsigned int debugCount = 0, transmitCount = 0;

// keep track of the timing of the web interface
long tic_web = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  Serial.print("\n[esp8266_imu_osc / ");
  Serial.print(__DATE__ " / " __TIME__);
  Serial.println("]");

  WiFi.hostname(host);
  WiFi.begin();

  Wire.begin(5, 4);  // SDA=D1=5, SCL=D2=4
  Wire.setClock(400000L);

  SPIFFS.begin();

  // Used for OSC
  Udp.begin(inPort);

  ledInit();

  if (loadConfig()) {
    ledYellow();
    delay(1000);
  }
  else {
    ledRed();
    delay(1000);
  }

  if (WiFi.status() != WL_CONNECTED)
    ledRed();

  WiFiManager wifiManager;
  // wifiManager.resetSettings();  // this is only needed when flashing a completely new ESP8266
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(host);
  Serial.println("connected");

  if (WiFi.status() == WL_CONNECTED)
    ledGreen();

#ifdef ENABLE_WEBINTERFACE
  // this serves all URIs that can be resolved to a file on the SPIFFS filesystem
  server.onNotFound(handleNotFound);

  server.on("/", HTTP_GET, []() {
    tic_web = millis();
    handleRedirect("/index");
  });

  server.on("/index", HTTP_GET, []() {
    tic_web = millis();
    handleStaticFile("/index.html");
  });

  server.on("/defaults", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleDefaults");
    handleStaticFile("/reload_success.html");
    delay(2000);
    ledRed();
    initialConfig();
    saveConfig();
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    WiFi.hostname(host);
    ESP.restart();
  });

  server.on("/reconnect", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleReconnect");
    handleStaticFile("/reload_success.html");
    delay(2000);
    ledRed();
    WiFiManager wifiManager;
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.startConfigPortal(host);
    Serial.println("connected");
    if (WiFi.status() == WL_CONNECTED)
      ledGreen();
  });

  server.on("/reset", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleReset");
    handleStaticFile("/reload_success.html");
    delay(2000);
    ledRed();
    ESP.restart();
  });

  server.on("/monitor", HTTP_GET, [] {
    tic_web = millis();
    handleStaticFile("/monitor.html");
  });

  server.on("/hello", HTTP_GET, [] {
    tic_web = millis();
    handleStaticFile("/hello.html");
  });

  server.on("/settings", HTTP_GET, [] {
    tic_web = millis();
    handleStaticFile("/settings.html");
  });

  server.on("/dir", HTTP_GET, [] {
    tic_web = millis();
    handleDirList();
  });

  server.on("/json", HTTP_PUT, [] {
    tic_web = millis();
    handleJSON();
  });

  server.on("/json", HTTP_POST, [] {
    tic_web = millis();
    handleJSON();
  });

  server.on("/json", HTTP_GET, [] {
    tic_web = millis();
    StaticJsonBuffer<300> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    CONFIG_TO_JSON(sensors, "sensors");
    CONFIG_TO_JSON(decimate, "decimate");
    S_CONFIG_TO_JSON(destination, "destination");
    CONFIG_TO_JSON(port, "port");
    root["version"] = version;
    root["uptime"]  = long(millis() / 1000);
    root["rate"]    = rate;
    String str;
    root.printTo(str);
    server.send(200, "application/json", str);
  });

  server.on("/update", HTTP_GET, [] {
    tic_web = millis();
    handleStaticFile("/update.html");
  });

  server.on("/update", HTTP_POST, handleUpdate1, handleUpdate2);

  // start the web server
  server.begin();
#endif

#ifdef ENABLE_MDNS
  // announce the hostname and web server through zeroconf
  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);
#endif

#ifdef ENABLE_IMU
  // Look for I2C devices on the bus
  I2Cscan();

  for (int i = 0; i < config.sensors; i++) {
    Serial.println("====================================================");
    Serial.println(String("initializing " + id[i]));
    Serial.println("====================================================");
    tca.select(i);
    mpu[i].begin();
    ahrs[i].begin();
  }
#endif

  Serial.println("====================================================");
  Serial.println("Setup done");
  Serial.println("====================================================");
}

void loop() {
  OSCBundle bundle;
  char msgId[16];
  float roll, yaw, pitch;
  float ax, ay, az, gx, gy, gz, mx, my, mz; // variables to hold latest MPU9250 sensor data values
  float temp = 20;

#ifdef ENABLE_WEBINTERFACE
  server.handleClient();
#endif

  if (WiFi.status() != WL_CONNECTED) {
    ledRed();
  }
  else if ((millis() - tic_web) < 5000) {
    // serving content on the http interface takes a lot of resources and
    // messes up the regular timing of the acquisition and transmission
    ledBlue();
  }

#ifdef ENABLE_IMU
  for (int i = 0; i < config.sensors; i++) {
    tca.select(i);
    while (mpu[i].newData() == 0)
      delay(1);

    Now = micros();
    deltat = ((Now - Last[i]) / 1000000.0f); // set integration time by time elapsed since last filter update

    mpu[i].readAccelData(&ax, &ay, &az);
    mpu[i].readGyroData(&gx, &gy, &gz);
    mpu[i].readMagData(&mx, &my, &mz);

    ahrs[i].update(ax, ay, az, gx, gy, gz, my, mx, mz, deltat);
    roll = ahrs[i].roll;
    yaw = ahrs[i].yaw;
    pitch = ahrs[i].pitch;

    if ((Now - lastTemperature) > 1000) {
      mpu[i].readTempData(&temp);
      String(id[i] + "/temp").toCharArray(msgId, 16); bundle.add(msgId).add(temp);
    }

    String(id[i] + "/a").toCharArray(msgId, 16); bundle.add(msgId).add(ax).add(ay).add(az);
    String(id[i] + "/g").toCharArray(msgId, 16); bundle.add(msgId).add(gx).add(gy).add(gz);
    String(id[i] + "/m").toCharArray(msgId, 16); bundle.add(msgId).add(mx).add(my).add(mz);
    String(id[i] + "/roll").toCharArray(msgId, 16); bundle.add(msgId).add(roll);
    String(id[i] + "/yaw").toCharArray(msgId, 16); bundle.add(msgId).add(yaw);
    String(id[i] + "/pitch").toCharArray(msgId, 16); bundle.add(msgId).add(pitch);
    String(id[i] + "/rate").toCharArray(msgId, 16); bundle.add(msgId).add(1000000.0f / (Now - Last[i]));
    String(id[i] + "/time").toCharArray(msgId, 16); bundle.add(msgId).add(Now / 1000000.0f);

    Last[i] = Now;
  } // for loop over the IMUs

  Now = millis();

  transmitCount++;
  if ((transmitCount % config.decimate) == 0) {
    outIp.fromString(config.destination);
    Udp.beginPacket(outIp, config.port);
    bundle.send(Udp);
    Udp.endPacket();
    bundle.empty();
    lastTransmit = Now;
    transmitCount = 0;
  }

  if ((Now - lastTemperature) > 1000) {
    lastTemperature = Now;
  }

  debugCount++;
  if (Debug && (Now - lastDisplay) > 1000) {
    // Update the debug information every second, independent of data rates
    long elapsedTime = Now - lastDisplay;
    rate = 1000.0f * debugCount / elapsedTime, 2;

    if (SerialDebug) {
      Serial.println("===============================================");
      Serial.println("MPU9250: ");
      Serial.println("===============================================");
      Serial.print("ax = ");  Serial.print((int)1000 * ax);
      Serial.print(" ay = "); Serial.print((int)1000 * ay);
      Serial.print(" az = "); Serial.print((int)1000 * az); Serial.println(" mg");
      Serial.print("gx = ");  Serial.print( gx, 2);
      Serial.print(" gy = "); Serial.print( gy, 2);
      Serial.print(" gz = "); Serial.print( gz, 2); Serial.println(" deg/s");
      Serial.print("mx = ");  Serial.print( (int)mx );
      Serial.print(" my = "); Serial.print( (int)my );
      Serial.print(" mz = "); Serial.print( (int)mz ); Serial.println(" mG");

      Serial.print("Gyro temperature = ");
      Serial.print(temp, 1);
      Serial.println(" degrees C"); // Print T values to tenths of degree C

      Serial.print("sensors = "); Serial.println(config.sensors);
      Serial.print("decimate = "); Serial.println(config.decimate);
      Serial.print("rate = "); Serial.print(rate); Serial.println(" Hz");
      Serial.print("count = "); Serial.println(debugCount);
      Serial.print("elapsedTime = "); Serial.print(elapsedTime); Serial.println(" ms");
    }

    // the following causes the green LED to blink on and off every second
    if (digitalRead(LED_G)) {
      ledBlack();
    }
    else {
      ledGreen();
    }

    lastDisplay = millis();
    debugCount = 0;
  }
#endif

} // loop
