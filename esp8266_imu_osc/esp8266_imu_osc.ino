#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <OSCBundle.h>

#include "tca9548a.h"
#include "mpu9250.h"
#include "ahrs.h"
#include "secret.h"

#define Debug true
#define SerialDebug true
#define maxNumberImu 4

tca9548a tca;
mpu9250 mpu[maxNumberImu];
ahrs ahrs[maxNumberImu];
String id[16] = {"imu0", "imu1", "imu2", "imu3", "imu4", "imu5", "imu6", "imu7", "imu8", "imu9", "imu10", "imu11", "imu12", "imu13", "imu14", "imu15"};

// Pin definitions
int myLed = LED_BUILTIN;

// UDP destination address
IPAddress outIp(192, 168, 1, 144);
const unsigned int inPort = 9000, outPort = 8000;
WiFiUDP Udp;

float deltat = 0.0f, sum = 0.0f;            // integration interval for both filter schemes
uint32_t Now = 0, Last[maxNumberImu];       // used to calculate integration interval
uint32_t lastDisplay = 0, lastTemperature = 0, lastTransmit = 0;
long debugCount = 0, transmitCount = 0;
int decimate = 10;

float ax, ay, az, gx, gy, gz, mx, my, mz; // variables to hold latest MPU9250 sensor data values
float temp = 20;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  Serial.print("\n[esp8266_imu_osc / ");
  Serial.print(__DATE__ " / " __TIME__);
  Serial.println("]");

  Wire.begin(D2, D1);
  Wire.setClock(400000L);

  // Set up the interrupt pin, its set as active high, push-pull
  pinMode(myLed, OUTPUT);
  digitalWrite(myLed, HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (count++ > 10) {
      Serial.println("reset");
      ESP.reset();
    }
    else
      Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Used for OSC
  Udp.begin(inPort);

  // Look for I2C devices on the bus
  I2Cscan();

  for (int i = 0; i < maxNumberImu; i++) {
    Serial.println("====================================================");
    Serial.println(String("initializing " + id[i]));
    Serial.println("====================================================");
    tca.select(i);
    mpu[i].begin();
    ahrs[i].begin();
  }
  Serial.println("====================================================");
  Serial.println("Setup done");
  Serial.println("====================================================");
}


void loop() {
  OSCBundle bundle;
  char msgId[16];
  float roll, yaw, pitch;

  for (int i = 0; i < maxNumberImu; i++) {
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
  debugCount++;

if ((transmitCount % decimate) == 0) {
    transmitCount = 0;
    lastTransmit = Now;
    Udp.beginPacket(outIp, outPort);
    bundle.send(Udp);
    Udp.endPacket();
  }
  bundle.empty();

  if ((Now - lastTemperature) > 1000) {
    lastTemperature = Now;
  }

  if (Debug && (Now - lastDisplay) > 1000) {
    // Update the debug information every second, independent of data rates

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

      long elapsedTime = Now - lastDisplay;
      Serial.print("rate = "); Serial.print(1000.0f * debugCount / elapsedTime, 2); Serial.println(" Hz");
      Serial.print("count = "); Serial.println(debugCount);
      Serial.print("elapsedTime = "); Serial.print(elapsedTime); Serial.println(" ms");
    }

    digitalWrite(myLed, !digitalRead(myLed));
    lastDisplay = millis();
    debugCount = 0;
  }

} // loop

