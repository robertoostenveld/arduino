#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <OSCBundle.h>

#include "mpu9250.h"
#include "tca9548a.h"
#include "secret.h"

// set to true to use Serial output for debugging
#define SerialDebug true

tca9548a tca;
mpu9250 mpu0, mpu1, mpu2, mpu3;

// Pin definitions
int myLed = LED_BUILTIN;

// UDP destination address
IPAddress outIp(192, 168, 1, 144);
const unsigned int inPort = 9000, outPort = 8000;
WiFiUDP Udp;

// global constants for 9 DoF fusion and AHRS (Attitude and Heading Reference System)
float GyroMeasError = PI * (40.0f / 180.0f);   // gyroscope measurement error in rads/s (start at 40 deg/s)
float GyroMeasDrift = PI * (0.0f  / 180.0f);   // gyroscope measurement drift in rad/s/s (start at 0.0 deg/s/s)

// There is a tradeoff in the beta parameter between accuracy and response speed.
// In the original Madgwick study, beta of 0.041 (corresponding to GyroMeasError of 2.7 degrees/s) was found to give optimal accuracy.
// However, with this value, the LSM9SD0 response time is about 10 seconds to a stable initial quaternion.
// Subsequent changes also require a longish lag time to a stable output, not fast enough for a quadcopter or robot car!
// By increasing beta (GyroMeasError) by about a factor of fifteen, the response time constant is reduced to ~2 sec
// I haven't noticed any reduction in solution accuracy. This is essentially the I coefficient in a PID control sense;
// the bigger the feedback coefficient, the faster the solution converges, usually at the expense of accuracy.
// In any case, this is the free parameter in the Madgwick filtering and fusion scheme.
float beta = sqrt(3.0f / 4.0f) * GyroMeasError;   // compute beta
float zeta = sqrt(3.0f / 4.0f) * GyroMeasDrift;   // compute zeta, the other free parameter in the Madgwick scheme usually set to a small or zero value
#define Kp 2.0f * 5.0f // these are the free parameters in the Mahony filter and fusion scheme, Kp for proportional feedback, Ki for integral
#define Ki 0.0f

uint32_t lastDisplay = 0, elapsedTime = 0, count = 0;  // used to control display output rate
float pitch, yaw, roll;
float a12, a22, a31, a32, a33;            // rotation matrix coefficients for Euler angles and gravity components
float deltat = 0.0f, sum = 0.0f;          // integration interval for both filter schemes
uint32_t lastUpdate = 0, firstUpdate = 0; // used to calculate integration interval
uint32_t Now = 0;                         // used to calculate integration interval

float ax, ay, az, gx, gy, gz, mx, my, mz; // variables to hold latest MPU9250 sensor data values
float temp = 20;

float lin_ax, lin_ay, lin_az;             // linear acceleration (acceleration with gravity component subtracted)
float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};    // vector to hold quaternion
float eInt[3] = {0.0f, 0.0f, 0.0f};       // vector to hold integral error for Mahony method


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

  tca.select(0);
  mpu0.setup();
  tca.select(1);
  mpu1.setup();
  tca.select(2);
  mpu2.setup();
  tca.select(3);
  mpu3.setup();
}



void loop() {
  tca.select(0);
  if (mpu0.newData()) {
    mpu0.readAccelData(&ax, &ay, &az);
    mpu0.readGyroData(&gx, &gy, &gz);
    mpu0.readMagData(&mx, &my, &mz);

    Now = micros();
    deltat = ((Now - lastUpdate) / 1000000.0f); // set integration time by time elapsed since last filter update
    lastUpdate = Now;

    elapsedTime += 1000.0f * deltat; // sum for averaging filter update rate
    count++;

    // Sensors x (y)-axis of the accelerometer/gyro is aligned with the y (x)-axis of the magnetometer;
    // the magnetometer z-axis (+ down) is misaligned with z-axis (+ up) of accelerometer and gyro!
    // We have to make some allowance for this orientation mismatch in feeding the output to the quaternion filter.
    // For the MPU9250+MS5637 Mini breakout the +x accel/gyro is North, then -y accel/gyro is East. So if we want te quaternions properly aligned
    // we need to feed into the Madgwick function Ax, -Ay, -Az, Gx, -Gy, -Gz, My, -Mx, and Mz. But because gravity is by convention
    // positive down, we need to invert the accel data, so we pass -Ax, Ay, Az, Gx, -Gy, -Gz, My, -Mx, and Mz into the Madgwick
    // function to get North along the accel +x-axis, East along the accel -y-axis, and Down along the accel -z-axis.
    // This orientation choice can be modified to allow any convenient (non-NED) orientation convention.
    // Pass gyro rate as rad/s
    MadgwickQuaternionUpdate(-ax, ay, az, gx * PI / 180.0f, -gy * PI / 180.0f, -gz * PI / 180.0f,  my, -mx, mz);
    // MahonyQuaternionUpdate(-ax, ay, az, gx * PI / 180.0f, -gy * PI / 180.0f, -gz * PI / 180.0f,  my, -mx, mz);

    // Define output variables from updated quaternion---these are Tait-Bryan angles, commonly used in aircraft orientation.
    // In this coordinate system, the positive z-axis is down toward Earth.
    // Yaw is the angle between Sensor x-axis and Earth magnetic North (or true North if corrected for local declination, looking down on the sensor positive yaw is counterclockwise.
    // Pitch is angle between sensor x-axis and Earth ground plane, toward the Earth is positive, up toward the sky is negative.
    // Roll is angle between sensor y-axis and Earth ground plane, y-axis up is positive roll.
    // These arise from the definition of the homogeneous rotation matrix constructed from quaternions.
    // Tait-Bryan angles as well as Euler angles are non-commutative; that is, to get the correct orientation the rotations must be
    // applied in the correct order which for this configuration is yaw, pitch, and then roll.
    // For more see http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles which has additional links.
    // Software AHRS:
    //   yaw   = atan2f(2.0f * (q[1] * q[2] + q[0] * q[3]), q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3]);
    //   pitch = -asinf(2.0f * (q[1] * q[3] - q[0] * q[2]));
    //   roll  = atan2f(2.0f * (q[0] * q[1] + q[2] * q[3]), q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3]);
    //   pitch *= 180.0f / PI;
    //   yaw   *= 180.0f / PI;
    //   yaw   += 13.8f; // Declination at Danville, California is 13 degrees 48 minutes and 47 seconds on 2014-04-04
    //   if(yaw < 0) yaw   += 360.0f; // Ensure yaw stays between 0 and 360
    //   roll  *= 180.0f / PI;
    a12    = 2.0f * (q[1] * q[2] + q[0] * q[3]);
    a22    = q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3];
    a31    = 2.0f * (q[0] * q[1] + q[2] * q[3]);
    a32    = 2.0f * (q[1] * q[3] - q[0] * q[2]);
    a33    = q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3];
    pitch  = -asinf(a32);
    roll   = atan2f(a31, a33);
    yaw    = atan2f(a12, a22);
    pitch *= 180.0f / PI;
    yaw   *= 180.0f / PI;
    yaw   += 1.5167f;           // Declination at Nijmegen, NL is 1 degrees 31 minutes and 00 seconds on 2017-12-16
    if (yaw < 0) yaw += 360.0f; // Ensure yaw stays between 0 and 360
    roll  *= 180.0f / PI;
    lin_ax = ax + a31;
    lin_ay = ay + a32;
    lin_az = az - a33;
  }

  OSCBundle bundle;
  bundle.add("/mpu9250/temp").add(temp);
  bundle.add("/mpu9250/a").add(ax).add(ay).add(az);
  bundle.add("/mpu9250/g").add(gx).add(gy).add(gz);
  bundle.add("/mpu9250/m").add(mx).add(my).add(mz);
  bundle.add("/mpu9250/q").add(q[0]).add(q[1]).add(q[2]).add(q[3]);
  bundle.add("/mpu9250/lin_a").add(lin_ax).add(lin_ay).add(lin_az);
  bundle.add("/mpu9250/roll").add(roll);
  bundle.add("/mpu9250/yaw").add(yaw);
  bundle.add("/mpu9250/pitch").add(pitch);
  bundle.add("/mpu9250/rate").add(1000.0f * count / elapsedTime);
  Udp.beginPacket(outIp, outPort);
  bundle.send(Udp);
  Udp.endPacket();
  bundle.empty();

  // Update the temperature and display debug information every 0.5 second, independent of data rates
  if (millis() - lastDisplay > 500) {

    mpu0.readTempData(&temp);  // Read the gyro adc values

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

      Serial.print("q0 = ");  Serial.print(q[0]);
      Serial.print(" qx = "); Serial.print(q[1]);
      Serial.print(" qy = "); Serial.print(q[2]);
      Serial.print(" qz = "); Serial.println(q[3]);

      Serial.print("Yaw, Pitch, Roll: ");
      Serial.print(yaw, 2);
      Serial.print(", ");
      Serial.print(pitch, 2);
      Serial.print(", ");
      Serial.println(roll, 2);

      Serial.print("Grav_x, Grav_y, Grav_z: ");
      Serial.print(-a31 * 1000, 2);
      Serial.print(", ");
      Serial.print(-a32 * 1000, 2);
      Serial.print(", ");
      Serial.print(a33 * 1000, 2);  Serial.println(" mg");
      Serial.print("Lin_ax, Lin_ay, Lin_az: ");
      Serial.print(lin_ax * 1000, 2);
      Serial.print(", ");
      Serial.print(lin_ay * 1000, 2);
      Serial.print(", ");
      Serial.print(lin_az * 1000, 2);  Serial.println(" mg");

      Serial.print("Gyro temperature = ");
      Serial.print(temp, 1);
      Serial.println(" degrees C"); // Print T values to tenths of degree C

      Serial.print("rate = "); Serial.print(1000.0f * count / elapsedTime, 2); Serial.println(" Hz");
      Serial.print("count = "); Serial.println(count);
      Serial.print("elapsedTime = "); Serial.println(elapsedTime);
    }

    digitalWrite(myLed, !digitalRead(myLed));
    lastDisplay = millis();
    count = 0;
    elapsedTime  = 0;
  }

} // loop


//===================================================================================================================
// I2C scan function
//===================================================================================================================

void I2Cscan() {
  byte error, address;
  uint16_t nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++ ) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}


//===================================================================================================================
// I2C read/write functions for the MPU9250 and AK8963 sensors
//===================================================================================================================

void writeByte(uint8_t address, uint8_t subAddress, uint8_t data) {
  Wire.beginTransmission(address);  // Initialize the Tx buffer
  Wire.write(subAddress);           // Put slave register address in Tx buffer
  Wire.write(data);                 // Put data in Tx buffer
  Wire.endTransmission();           // Send the Tx buffer
}

uint8_t readByte(uint8_t address, uint8_t subAddress) {
  uint8_t data; // `data` will store the register data
  Wire.beginTransmission(address);        // Initialize the Tx buffer
  Wire.write(subAddress);	                // Put slave register address in Tx buffer
  Wire.endTransmission();                 // Send the Tx buffer, but send a restart to keep connection alive
  //	Wire.endTransmission(false);        // Send the Tx buffer, but send a restart to keep connection alive
  //	Wire.requestFrom(address, 1);       // Read one byte from slave register address
  Wire.requestFrom(address, (size_t) 1);  // Read one byte from slave register address
  data = Wire.read();                     // Fill Rx buffer with result
  return data;                            // Return data read from slave register
}

void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest) {
  Wire.beginTransmission(address);      // Initialize the Tx buffer
  Wire.write(subAddress);               // Put slave register address in Tx buffer
  Wire.endTransmission();               // Send the Tx buffer, but send a restart to keep connection alive
  // Wire.endTransmission(false);       // Send the Tx buffer, but send a restart to keep connection alive
  uint8_t i = 0;
  // Wire.requestFrom(address, count);  // Read bytes from slave register address
  Wire.requestFrom(address, (size_t) count);  // Read bytes from slave register address
  while (Wire.available()) {
    dest[i++] = Wire.read();
  }         // Put read results in the Rx buffer
}


