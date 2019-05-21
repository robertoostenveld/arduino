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
#include <ESP8266WiFi.h>         // https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <WiFiUDP.h>
#include <OSCBundle.h>
#include <FS.h>

// from https://github.com/robertoostenveld/SparkFun_MPU-9250_Breakout_Arduino_Library
#include <MPU9250.h>
#include <quaternionFilters.h>

#include "tca9548a.h"
#include "setup_ota.h"
#include "rgb_led.h"
#include "I2Cscan.h"

#define debugLevel 1          // 0 = silent, 1 = blink led, 2 = print on serial console
#define maxSensors 8

// this allows some sections of the code to be disabled for debugging purposes
#define ENABLE_WEBINTERFACE
#define ENABLE_MDNS
#define ENABLE_IMU

// Use either of these to select which I2C address your device is using
#define MPU9250_ADDRESS MPU9250_ADDRESS_AD0
// #define MPU9250_ADDRESS MPU9250_ADDRESS_AD1

Config config;
ESP8266WebServer server(80);
const char* host = "IMU-OSC";
const char* version = __DATE__ " / " __TIME__;

tca9548a tca;
MPU9250 mpu[maxSensors];
String id[maxSensors] = {"imu1", "imu2", "imu3", "imu4", "imu5", "imu6", "imu7", "imu8"};

// UDP destination address, these will be changed according to the configuration
IPAddress outIp(192, 168, 1, 100);
const unsigned int inPort = 9000, outPort = 8000;
WiFiUDP Udp;

float rate = 0.0f;
float deltat = 0.0f, sum = 0.0f;          // integration interval for both filter schemes
uint32_t Now = 0, Last[maxSensors];       // used to calculate integration interval
uint32_t lastDisplay = 0, lastTemperature = 0, lastTransmit = 0;
unsigned int debugCount = 0, measurementCount = 0;

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

  pinMode(0, OUTPUT);     // RESET=D3=0
  digitalWrite(0, LOW);   // reset on
  delay(10);
  digitalWrite(0, HIGH);  // reset off

  Wire.begin(5, 4);       // SDA=D1=5, SCL=D2=4
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
    CONFIG_TO_JSON(calibrate, "calibrate");
    CONFIG_TO_JSON(raw, "raw");
    CONFIG_TO_JSON(ahrs, "ahrs");
    CONFIG_TO_JSON(quaternion, "quaternion");
    CONFIG_TO_JSON(temperature, "temperature");
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

  byte status = 0;
  for (int i = 0; i < config.sensors; i++) {
    Serial.println("====================================================");
    Serial.println(String("initializing " + id[i]));
    tca.select(i);

    // Read the WHO_AM_I register, this is a good test of communication
    byte c = mpu[i].readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250);
    Serial.print(F("MPU9250 I am 0x"));
    Serial.print(c, HEX);
    Serial.print(F(" I should be 0x"));
    Serial.println(0x71, HEX);

    // WHO_AM_I should be 0x71 for MPU9250 or 0x73 for MPU9255
    if ((c == 0x71) || (c == 0x73)) {
      Serial.println(F("MPU9250 is online..."));

      // Start by performing self test and reporting values
      mpu[i].MPU9250SelfTest(mpu[i].selfTest);
      Serial.print(F("x-axis self test: acceleration trim within : "));
      Serial.print(mpu[i].selfTest[0], 1); Serial.println("% of factory value");
      Serial.print(F("y-axis self test: acceleration trim within : "));
      Serial.print(mpu[i].selfTest[1], 1); Serial.println("% of factory value");
      Serial.print(F("z-axis self test: acceleration trim within : "));
      Serial.print(mpu[i].selfTest[2], 1); Serial.println("% of factory value");
      Serial.print(F("x-axis self test: gyration trim within : "));
      Serial.print(mpu[i].selfTest[3], 1); Serial.println("% of factory value");
      Serial.print(F("y-axis self test: gyration trim within : "));
      Serial.print(mpu[i].selfTest[4], 1); Serial.println("% of factory value");
      Serial.print(F("z-axis self test: gyration trim within : "));
      Serial.print(mpu[i].selfTest[5], 1); Serial.println("% of factory value");

      // Calibrate gyro and accelerometers, load biases in bias registers
      mpu[i].calibrateMPU9250(mpu[i].gyroBias, mpu[i].accelBias);

      mpu[i].initMPU9250();
      // Initialize device for active mode read of acclerometer, gyroscope, and temperature
      Serial.println("MPU9250 initialized for active data mode....");

      // Read the WHO_AM_I register of the magnetometer, this is a good test of communication
      byte d = mpu[i].readByte(AK8963_ADDRESS, WHO_AM_I_AK8963);
      Serial.print("AK8963 ");
      Serial.print("I am 0x");
      Serial.print(d, HEX);
      Serial.print(" I should be 0x");
      Serial.println(0x48, HEX);

      if (d != 0x48) {
        status++;
      }

      // Get magnetometer calibration from AK8963 ROM
      mpu[i].initAK8963(mpu[i].factoryMagCalibration);
      // Initialize device for active mode read of magnetometer
      Serial.println("AK8963 initialized for active data mode....");

      //  Serial.println("Calibration values: ");
      Serial.print("X-Axis factory magnetometer adjustment value ");
      Serial.println(mpu[i].factoryMagCalibration[0], 2);
      Serial.print("Y-Axis factory magnetometer adjustment value ");
      Serial.println(mpu[i].factoryMagCalibration[1], 2);
      Serial.print("Z-Axis factory magnetometer adjustment value ");
      Serial.println(mpu[i].factoryMagCalibration[2], 2);

      // Get sensor resolutions, only need to do this once
      mpu[i].getAres();
      mpu[i].getGres();
      mpu[i].getMres();

      if (config.calibrate == 1) {
        // The next call delays for 4 seconds, and then records about 15 seconds of data to calculate bias and scale.
        mpu[i].magCalMPU9250(mpu[i].magBias, mpu[i].magScale);

        Serial.println("AK8963 mag biases (mG)");
        Serial.println(mpu[i].magBias[0]);
        Serial.println(mpu[i].magBias[1]);
        Serial.println(mpu[i].magBias[2]);

        Serial.println("AK8963 mag scale (mG)");
        Serial.println(mpu[i].magScale[0]);
        Serial.println(mpu[i].magScale[1]);
        Serial.println(mpu[i].magScale[2]);
      }

    } // if (c == 0x71)
    else {
      status++;
    }

    if (status) {
      Serial.println("not present");
      config.sensors = i;  // the index starts at 0
      break;
    }
  } // for each sensor
  Serial.println("====================================================");
#endif

  Serial.println("====================================================");
  Serial.println("Setup done");
  Serial.println("====================================================");
}

void loop() {
  OSCBundle bundle;
  char msgId[16];
  const float *q;

#ifdef ENABLE_WEBINTERFACE
  server.handleClient();
#endif

  if (WiFi.status() != WL_CONNECTED) {
    ledRed();
  }
  else if ((millis() - tic_web) < 3000) {
    // serving content on the http interface takes a lot of resources and
    // messes up the regular timing of the acquisition and transmission
    ledBlue();
  }

#ifdef ENABLE_IMU
  for (int i = 0; i < config.sensors; i++) {
    tca.select(i);

    while (!(mpu[i].readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01))
      delay(1);

    mpu[i].readAccelData(mpu[i].accelCount);  // Read the x/y/z adc values
    // Now we'll calculate the accleration value into actual g's
    // This depends on scale being set
    mpu[i].ax = (float)mpu[i].accelCount[0] * mpu[i].aRes; // - mpu[i].accelBias[0];
    mpu[i].ay = (float)mpu[i].accelCount[1] * mpu[i].aRes; // - mpu[i].accelBias[1];
    mpu[i].az = (float)mpu[i].accelCount[2] * mpu[i].aRes; // - mpu[i].accelBias[2];

    mpu[i].readGyroData(mpu[i].gyroCount);  // Read the x/y/z adc values
    // Calculate the gyro value into actual degrees per second
    // This depends on scale being set
    mpu[i].gx = (float)mpu[i].gyroCount[0] * mpu[i].gRes; // - mpu[i].gyroBias[0];
    mpu[i].gy = (float)mpu[i].gyroCount[1] * mpu[i].gRes; // - mpu[i].gyroBias[1];
    mpu[i].gz = (float)mpu[i].gyroCount[2] * mpu[i].gRes; // - mpu[i].gyroBias[2];

    mpu[i].readMagData(mpu[i].magCount);  // Read the x/y/z adc values
    // Calculate the magnetometer values in milliGauss
    // Get actual magnetometer value, this depends on scale being set
    // Include factory calibration per data sheet and user environmental corrections
    mpu[i].mx = (float)mpu[i].magCount[0] * mpu[i].mRes * mpu[i].factoryMagCalibration[0] - mpu[i].magBias[0];
    mpu[i].my = (float)mpu[i].magCount[1] * mpu[i].mRes * mpu[i].factoryMagCalibration[1] - mpu[i].magBias[1];
    mpu[i].mz = (float)mpu[i].magCount[2] * mpu[i].mRes * mpu[i].factoryMagCalibration[2] - mpu[i].magBias[2];

    if (config.raw) {
      String(id[i] + "/a").toCharArray(msgId, 16); bundle.add(msgId).add(mpu[i].ax).add(mpu[i].ay).add(mpu[i].az);
      String(id[i] + "/g").toCharArray(msgId, 16); bundle.add(msgId).add(mpu[i].gx).add(mpu[i].gy).add(mpu[i].gz);
      String(id[i] + "/m").toCharArray(msgId, 16); bundle.add(msgId).add(mpu[i].mx).add(mpu[i].my).add(mpu[i].mz);
    }

    if (config.calibrate == 2) {
      mpu[i].magContinuousCalMPU9250(mpu[i].magCount, mpu[i].magBias, mpu[i].magScale);
    }

    if (config.ahrs || config.quaternion) {
      // Must be called before updating quaternions!
      mpu[i].updateTime();

      // Sensors x (y)-axis of the accelerometer is aligned with the y (x)-axis of
      // the magnetometer; the magnetometer z-axis (+ down) is opposite to z-axis
      // (+ up) of accelerometer and gyro! We have to make some allowance for this
      // orientationmismatch in feeding the output to the quaternion filter. For the
      // MPU-9250, we have chosen a magnetic rotation that keeps the sensor forward
      // along the x-axis just like in the LSM9DS0 sensor. This rotation can be
      // modified to allow any convenient orientation convention. This is ok by
      // aircraft orientation standards! Pass gyro rate as rad/s

      // MadgwickQuaternionUpdate(mpu[i].ax, mpu[i].ay, mpu[i].az, mpu[i].gx * DEG_TO_RAD, mpu[i].gy * DEG_TO_RAD, mpu[i].gz * DEG_TO_RAD, mpu[i].my, mpu[i].mx, -mpu[i].mz, mpu[i].deltat);
      MahonyQuaternionUpdate(mpu[i].ax, mpu[i].ay, mpu[i].az, mpu[i].gx * DEG_TO_RAD, mpu[i].gy * DEG_TO_RAD, mpu[i].gz * DEG_TO_RAD, mpu[i].my, mpu[i].mx, -mpu[i].mz, mpu[i].deltat);

      // Define output variables from updated quaternion---these are Tait-Bryan
      // angles, commonly used in aircraft orientation. In this coordinate system,
      // the positive z-axis is down toward Earth. Yaw is the angle between Sensor
      // x-axis and Earth magnetic North (or true North if corrected for local
      // declination, looking down on the sensor positive yaw is counterclockwise.
      // Pitch is angle between sensor x-axis and Earth ground plane, toward the
      // Earth is positive, up toward the sky is negative. Roll is angle between
      // sensor y-axis and Earth ground plane, y-axis up is positive roll. These
      // arise from the definition of the homogeneous rotation matrix constructed
      // from quaternions. Tait-Bryan angles as well as Euler angles are
      // non-commutative; that is, the get the correct orientation the rotations
      // must be applied in the correct order which for this configuration is yaw,
      // pitch, and then roll.
      // For more see
      // http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
      // which has additional links.

      q = getQ();
      mpu[i].yaw    = atan2(2.0f * (q[1] * q[2] + q[0] * q[3]), q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3]);
      mpu[i].pitch  = -asin(2.0f * (q[1] * q[3] - q[0] * q[2]));
      mpu[i].roll   = atan2(2.0f * (q[0] * q[1] + q[2] * q[3]), q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3]);
      mpu[i].yaw   *= RAD_TO_DEG;
      mpu[i].pitch *= RAD_TO_DEG;
      mpu[i].roll  *= RAD_TO_DEG;

      // See http://www.ngdc.noaa.gov/geomag-web/#declination
      // Declination of SparkFun Electronics (40°05'26.6"N 105°11'05.9"W) is 8° 30' E  ± 0° 21' (or 8.5°) on 2016-07-19
      // mpu[i].yaw  -= 8.5;
      // Declination at Nijmegen, NL is 1 degrees 31 minutes and 00 seconds on 2017-12-16
      mpu[i].yaw  -= 1.5167;

      if (config.ahrs) {
        String(id[i] + "/roll").toCharArray(msgId, 16); bundle.add(msgId).add(mpu[i].roll);
        String(id[i] + "/yaw").toCharArray(msgId, 16); bundle.add(msgId).add(mpu[i].yaw);
        String(id[i] + "/pitch").toCharArray(msgId, 16); bundle.add(msgId).add(mpu[i].pitch);
      }
      if (config.quaternion) {
        String(id[i] + "/q").toCharArray(msgId, 16); bundle.add(msgId).add(q[0]).add(q[1]).add(q[2]).add(q[3]);
      }
    } // if ahrs or quaternion

    if (config.temperature) {
      mpu[i].tempCount = mpu[i].readTempData();  // Read the adc values
      mpu[i].temperature = ((float) mpu[i].tempCount) / 333.87 + 21.0;  // Temperature in degrees Centigrade
      String(id[i] + "/temp").toCharArray(msgId, 16); bundle.add(msgId).add(mpu[i].temperature);
    }

    // this section is in micros
    Now = micros();
    rate = 1000000.0f / (Now - Last[i]);
    String(id[i] + "/rate").toCharArray(msgId, 16); bundle.add(msgId).add(rate);
    String(id[i] + "/time").toCharArray(msgId, 16); bundle.add(msgId).add(Now / 1000000.0f);
    Last[i] = Now;

    // the previous section was in micros, the following section in millis
    Now = millis();

    measurementCount++;
    if ((measurementCount % config.decimate) == 0) {
      // only send every Nth measurement
      outIp.fromString(config.destination);
      Udp.beginPacket(outIp, config.port);
      bundle.send(Udp);
      Udp.endPacket();
      bundle.empty();
      lastTransmit = Now;
      measurementCount = 0;
    }

    debugCount++;
    if ((debugLevel > 0) && (Now - lastDisplay) > 1000) {
      // Update the debug information every second, independent of data rates
      long elapsedTime = Now - lastDisplay;
      rate = 1000.0f * debugCount / elapsedTime, 2;

      if (debugLevel > 1) {
        // Print acceleration values in milligs!
        Serial.print("X-acceleration: "); Serial.print(1000 * mpu[i].ax);
        Serial.print(" mg ");
        Serial.print("Y-acceleration: "); Serial.print(1000 * mpu[i].ay);
        Serial.print(" mg ");
        Serial.print("Z-acceleration: "); Serial.print(1000 * mpu[i].az);
        Serial.println(" mg ");

        // Print gyro values in degree/sec
        Serial.print("X-gyro rate: "); Serial.print(mpu[i].gx, 3);
        Serial.print(" degrees/sec ");
        Serial.print("Y-gyro rate: "); Serial.print(mpu[i].gy, 3);
        Serial.print(" degrees/sec ");
        Serial.print("Z-gyro rate: "); Serial.print(mpu[i].gz, 3);
        Serial.println(" degrees/sec");

        // Print mag values in milliGauss
        Serial.print("X-mag field: "); Serial.print(mpu[i].mx);
        Serial.print(" mG ");
        Serial.print("Y-mag field: "); Serial.print(mpu[i].my);
        Serial.print(" mG ");
        Serial.print("Z-mag field: "); Serial.print(mpu[i].mz);
        Serial.println(" mG");

        if (config.ahrs) {
          // Print AHRS values in degrees
          Serial.print("Yaw, Pitch, Roll: ");
          Serial.print(mpu[i].yaw, 2);
          Serial.print(", ");
          Serial.print(mpu[i].pitch, 2);
          Serial.print(", ");
          Serial.print(mpu[i].roll, 2);
          Serial.println(" degrees");
        } // if (config.ahrs)

        if (config.quaternion) {
          // Print quaternion values
          Serial.print("Quaternion: ");
          Serial.print(q[0], 4);
          Serial.print(", ");
          Serial.print(q[1], 4);
          Serial.print(", ");
          Serial.print(q[2], 4);
          Serial.print(", ");
          Serial.print(q[3], 4);
          Serial.println("");
        } // if (config.ahrs)

        if (config.temperature) {
          Serial.print("Temperature: ");
          Serial.print(mpu[i].temperature, 1);
          Serial.println(" degrees C");
        }
      } // if debugLevel>1

      // the following causes the green LED to blink on and off every second
      if (digitalRead(LED_G)) {
        ledBlack();
      }
      else {
        ledGreen();
      }

      lastDisplay = millis();
      debugCount = 0;
    } // if debugLevel>0
  } // for each sensor
#endif

} // loop
