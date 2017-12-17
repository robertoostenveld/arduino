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
#define SerialDebug false  

// Pin definitions
int myLed = LED_BUILTIN;

// UDP destination address
IPAddress outIp(192, 168, 1, 144);
const unsigned int inPort = 9000, outPort = 8000;
WiFiUDP Udp;

uint8_t MPU9250Gscale = GFS_250DPS;
uint8_t MPU9250Ascale = AFS_2G;
uint8_t MPU9250Mscale = MFS_16BITS;             // Choose either 14-bit or 16-bit magnetometer resolution
uint8_t MPU9250Mmode = 0x06;                    // 2 for 8 Hz, 6 for 100 Hz continuous magnetometer data read
float MPU9250aRes, MPU9250gRes, MPU9250mRes;    // scale resolutions per LSB for the sensors

int16_t accelCount[3];      // Stores the 16-bit signed accelerometer sensor output
int16_t gyroCount[3];       // Stores the 16-bit signed gyro sensor output
int16_t magCount[3];        // Stores the 16-bit signed magnetometer sensor output
float magCalibration[3] = {0, 0, 0};  // Factory mag calibration and mag bias
float MPU9250gyroBias[3] = {0, 0, 0}, MPU9250accelBias[3] = {0, 0, 0}, MPU9250magBias[3] = {0, 0, 0};      // Bias corrections for gyro and accelerometer
float temperature;                          // Stores the MPU9250 gyro internal chip temperature in degrees Celsius
float SelfTest[6];                          // Holds results of gyro and accelerometer self test

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
float lin_ax, lin_ay, lin_az;             // linear acceleration (acceleration with gravity component subtracted)
float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};    // vector to hold quaternion
float eInt[3] = {0.0f, 0.0f, 0.0f};       // vector to hold integral error for Mahony method


void setup() {
  Wire.begin(D2, D1);
  Wire.setClock(400000L);
  tcaselect(0);

  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  // Set up the interrupt pin, its set as active high, push-pull
  pinMode(myLed, OUTPUT);
  digitalWrite(myLed, HIGH);

  Serial.print("\n[esp8266_imu_osc / ");
  Serial.print(__DATE__ " / " __TIME__);
  Serial.println("]");

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

  Udp.begin(inPort);

  // Look for I2C devices on the bus
  I2Cscan();

  // Read the WHO_AM_I register, this is a good test of communication
  Serial.println("MPU9250 9-axis motion sensor...");
  byte c = readByte(MPU9250_ADDRESS, MPU9250_WHO_AM_I);
  Serial.print("MPU9250 "); Serial.print("I am "); Serial.print(c, HEX); Serial.print(", I should be "); Serial.println(0x71, HEX);
  delay(1000);

  if (c == 0x71) {
    Serial.println("MPU9250 is online...");

    MPU9250SelfTest(SelfTest); // Start by performing self test and reporting values
    Serial.println("MPU9250 Self Test:");
    Serial.print("x-axis self test: acceleration trim within : "); Serial.print(SelfTest[0], 1); Serial.println("% of factory value");
    Serial.print("y-axis self test: acceleration trim within : "); Serial.print(SelfTest[1], 1); Serial.println("% of factory value");
    Serial.print("z-axis self test: acceleration trim within : "); Serial.print(SelfTest[2], 1); Serial.println("% of factory value");
    Serial.print("x-axis self test: gyration trim within : "); Serial.print(SelfTest[3], 1); Serial.println("% of factory value");
    Serial.print("y-axis self test: gyration trim within : "); Serial.print(SelfTest[4], 1); Serial.println("% of factory value");
    Serial.print("z-axis self test: gyration trim within : "); Serial.print(SelfTest[5], 1); Serial.println("% of factory value");
    delay(1000);

    // get sensor resolutions, only need to do this once
    MPU9250getAres();
    MPU9250getGres();
    MPU9250getMres();

    Serial.println("Calibrate MPU9250 gyro and accel");
    accelgyrocalMPU9250(MPU9250gyroBias, MPU9250accelBias); // Calibrate gyro and accelerometers, load biases in bias registers
    Serial.println("accel biases (mg)"); Serial.println(1000.*MPU9250accelBias[0]); Serial.println(1000.*MPU9250accelBias[1]); Serial.println(1000.*MPU9250accelBias[2]);
    Serial.println("gyro biases (dps)"); Serial.println(MPU9250gyroBias[0]); Serial.println(MPU9250gyroBias[1]); Serial.println(MPU9250gyroBias[2]);
    delay(1000);

    initMPU9250();
    Serial.println("MPU9250 initialized for active data mode...."); // Initialize device for active mode read of acclerometer, gyroscope, and temperature

    // Read the WHO_AM_I register of the magnetometer, this is a good test of communication
    byte d = readByte(AK8963_ADDRESS, AK8963_WHO_AM_I);  // Read WHO_AM_I register for AK8963
    Serial.print("AK8963 "); Serial.print("I am "); Serial.print(d, HEX); Serial.print(", I should be "); Serial.println(0x48, HEX);
    delay(1000);

    // Get magnetometer calibration from AK8963 ROM
    initAK8963(magCalibration); Serial.println("AK8963 initialized for active data mode...."); // Initialize device for active mode read of magnetometer

    magcalMPU9250(MPU9250magBias);
    Serial.println("AK8963 mag biases (mG)"); Serial.println(MPU9250magBias[0]); Serial.println(MPU9250magBias[1]); Serial.println(MPU9250magBias[2]);
    delay(1000);

    if (SerialDebug) {
      Serial.print("X-Axis sensitivity adjustment value "); Serial.println(magCalibration[0], 2);
      Serial.print("Y-Axis sensitivity adjustment value "); Serial.println(magCalibration[1], 2);
      Serial.print("Z-Axis sensitivity adjustment value "); Serial.println(magCalibration[2], 2);
    }
  }
  else {
    Serial.print("Could not connect to MPU9250: 0x");
    Serial.println(c, HEX);
    while (1) ; // Loop forever if communication doesn't happen
  }

} // setup


void loop() {
  // If intPin of MPU9250 goes high, all data registers have new data
  if (readByte(MPU9250_ADDRESS, MPU9250_INT_STATUS) & 0x01) {

    MPU9250readAccelData(accelCount);  // Read the x/y/z adc values
    // Calculate the accleration value into actual g's
    ax = (float)accelCount[0] * MPU9250aRes - MPU9250accelBias[0]; // get actual g value, this depends on scale being set
    ay = (float)accelCount[1] * MPU9250aRes - MPU9250accelBias[1];
    az = (float)accelCount[2] * MPU9250aRes - MPU9250accelBias[2];

    MPU9250readGyroData(gyroCount);  // Read the x/y/z adc values
    // Calculate the gyro value into actual degrees per second
    gx = (float)gyroCount[0] * MPU9250gRes; // get actual gyro value, this depends on scale being set
    gy = (float)gyroCount[1] * MPU9250gRes;
    gz = (float)gyroCount[2] * MPU9250gRes;

    MPU9250readMagData(magCount);  // Read the x/y/z adc values
    // Calculate the magnetometer values in milliGauss, include factory calibration per data sheet and user environmental corrections
    mx = (float)magCount[0] * MPU9250mRes * magCalibration[0] - MPU9250magBias[0]; // get actual magnetometer value, this depends on scale being set
    my = (float)magCount[1] * MPU9250mRes * magCalibration[1] - MPU9250magBias[1];
    mz = (float)magCount[2] * MPU9250mRes * magCalibration[2] - MPU9250magBias[2];
  } // if new data

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
  // Tait-Bryan angles as well as Euler angles are non-commutative; that is, the get the correct orientation the rotations must be
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
  a12 =   2.0f * (q[1] * q[2] + q[0] * q[3]);
  a22 =   q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3];
  a31 =   2.0f * (q[0] * q[1] + q[2] * q[3]);
  a32 =   2.0f * (q[1] * q[3] - q[0] * q[2]);
  a33 =   q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3];
  pitch = -asinf(a32);
  roll  = atan2f(a31, a33);
  yaw   = atan2f(a12, a22);
  pitch *= 180.0f / PI;
  yaw   *= 180.0f / PI;
  yaw   += 1.5167f; // Declination at Nijmegen, NL is 1 degrees 31 minutes and 00 seconds on 2017-12-16
  if (yaw < 0) yaw += 360.0f; // Ensure yaw stays between 0 and 360
  roll  *= 180.0f / PI;
  lin_ax = ax + a31;
  lin_ay = ay + a32;
  lin_az = az - a33;

  OSCBundle bundle;
  bundle.add("/mpu9250/temperature").add(temperature);
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

  // Update the temperature and display debug information every 0.5, independent of data rates
  if (millis() - lastDisplay > 500) {

    int tempCount = MPU9250readTempData();  // Read the gyro adc values
    temperature = ((float) tempCount) / 333.87 + 21.0; // Gyro chip temperature in degrees Centigrade

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
      Serial.print(temperature, 1);
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
//====== Set of useful function to access acceleration. gyroscope, magnetometer, and temperature data
//===================================================================================================================

void MPU9250getMres() {
  switch (MPU9250Mscale)  {
    // Possible magnetometer scales (and their register bit settings) are:
    // 14 bit resolution (0) and 16 bit resolution (1)
    case MFS_14BITS:
      MPU9250mRes = 10.*4912. / 8190.; // Proper scale to return milliGauss
      break;
    case MFS_16BITS:
      MPU9250mRes = 10.*4912. / 32760.0; // Proper scale to return milliGauss
      break;
  }
}

void MPU9250getGres() {
  switch (MPU9250Gscale) {
    // Possible gyro scales (and their register bit settings) are:
    // 250 DPS (00), 500 DPS (01), 1000 DPS (10), and 2000 DPS  (11).
    // Here's a bit of an algorith to calculate DPS/(ADC tick) based on that 2-bit value:
    case GFS_250DPS:
      MPU9250gRes = 250.0 / 32768.0;
      break;
    case GFS_500DPS:
      MPU9250gRes = 500.0 / 32768.0;
      break;
    case GFS_1000DPS:
      MPU9250gRes = 1000.0 / 32768.0;
      break;
    case GFS_2000DPS:
      MPU9250gRes = 2000.0 / 32768.0;
      break;
  }
}

void MPU9250getAres() {
  switch (MPU9250Ascale) {
    // Possible accelerometer scales (and their register bit settings) are:
    // 2 Gs (00), 4 Gs (01), 8 Gs (10), and 16 Gs  (11).
    // Here's a bit of an algorith to calculate DPS/(ADC tick) based on that 2-bit value:
    case AFS_2G:
      MPU9250aRes = 2.0 / 32768.0;
      break;
    case AFS_4G:
      MPU9250aRes = 4.0 / 32768.0;
      break;
    case AFS_8G:
      MPU9250aRes = 8.0 / 32768.0;
      break;
    case AFS_16G:
      MPU9250aRes = 16.0 / 32768.0;
      break;
  }
}

void MPU9250readAccelData(int16_t * destination) {
  uint8_t rawData[6];  // x/y/z accel register data stored here
  readBytes(MPU9250_ADDRESS, MPU9250_ACCEL_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers into data array
  destination[0] = ((int16_t)rawData[0] << 8) | rawData[1] ;  // Turn the MSB and LSB into a signed 16-bit value
  destination[1] = ((int16_t)rawData[2] << 8) | rawData[3] ;
  destination[2] = ((int16_t)rawData[4] << 8) | rawData[5] ;
}


void MPU9250readGyroData(int16_t * destination) {
  uint8_t rawData[6];  // x/y/z gyro register data stored here
  readBytes(MPU9250_ADDRESS, MPU9250_GYRO_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers sequentially into data array
  destination[0] = ((int16_t)rawData[0] << 8) | rawData[1] ;  // Turn the MSB and LSB into a signed 16-bit value
  destination[1] = ((int16_t)rawData[2] << 8) | rawData[3] ;
  destination[2] = ((int16_t)rawData[4] << 8) | rawData[5] ;
}

void MPU9250readMagData(int16_t * destination) {
  uint8_t rawData[7];  // x/y/z gyro register data, ST2 register stored here, must read ST2 at end of data acquisition
  if (readByte(AK8963_ADDRESS, AK8963_ST1) & 0x01) { // wait for magnetometer data ready bit to be set
    readBytes(AK8963_ADDRESS, AK8963_XOUT_L, 7, &rawData[0]);  // Read the six raw data and ST2 registers sequentially into data array
    uint8_t c = rawData[6]; // End data read by reading ST2 register
    if (!(c & 0x08)) { // Check if magnetic sensor overflow set, if not then report data
      destination[0] = ((int16_t)rawData[1] << 8) | rawData[0] ;  // Turn the MSB and LSB into a signed 16-bit value
      destination[1] = ((int16_t)rawData[3] << 8) | rawData[2] ;  // Data stored as little Endian
      destination[2] = ((int16_t)rawData[5] << 8) | rawData[4] ;
    }
  }
}

int16_t MPU9250readTempData() {
  uint8_t rawData[2];  // x/y/z gyro register data stored here
  readBytes(MPU9250_ADDRESS, MPU9250_TEMP_OUT_H, 2, &rawData[0]); // Read the two raw data registers sequentially into data array
  return ((int16_t)rawData[0] << 8) | rawData[1] ;  // Turn the MSB and LSB into a 16-bit value
}

void initAK8963(float * destination) {
  // First extract the factory calibration for each magnetometer axis
  uint8_t rawData[3];  // x/y/z gyro calibration data stored here
  writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x00); // Power down magnetometer
  delay(10);
  writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x0F); // Enter Fuse ROM access mode
  delay(10);
  readBytes(AK8963_ADDRESS, AK8963_ASAX, 3, &rawData[0]);  // Read the x-, y-, and z-axis calibration values
  destination[0] =  (float)(rawData[0] - 128) / 256. + 1.; // Return x-axis sensitivity adjustment values, etc.
  destination[1] =  (float)(rawData[1] - 128) / 256. + 1.;
  destination[2] =  (float)(rawData[2] - 128) / 256. + 1.;
  writeByte(AK8963_ADDRESS, AK8963_CNTL, 0x00); // Power down magnetometer
  delay(10);
  // Configure the magnetometer for continuous read and highest resolution
  // set Mscale bit 4 to 1 (0) to enable 16 (14) bit resolution in CNTL register,
  // and enable continuous mode data acquisition Mmode (bits [3:0]), 0010 for 8 Hz and 0110 for 100 Hz sample rates
  writeByte(AK8963_ADDRESS, AK8963_CNTL, MPU9250Mscale << 4 | MPU9250Mmode); // Set magnetometer data resolution and sample ODR
  delay(10);
}


void initMPU9250() {
  // wake up device
  writeByte(MPU9250_ADDRESS, MPU9250_PWR_MGMT_1, 0x00); // Clear sleep mode bit (6), enable all sensors
  delay(100); // Wait for all registers to reset

  // get stable time source
  writeByte(MPU9250_ADDRESS, MPU9250_PWR_MGMT_1, 0x01);  // Auto select clock source to be PLL gyroscope reference if ready else
  delay(200);

  // Configure Gyro and Thermometer
  // Disable FSYNC and set thermometer and gyro bandwidth to 41 and 42 Hz, respectively;
  // minimum delay time for this setting is 5.9 ms, which means sensor fusion update rates cannot
  // be higher than 1 / 0.0059 = 170 Hz
  // DLPF_CFG = bits 2:0 = 011; this limits the sample rate to 1000 Hz for both
  // With the MPU9250, it is possible to get gyro sample rates of 32 kHz (!), 8 kHz, or 1 kHz
  writeByte(MPU9250_ADDRESS, MPU9250_CONFIG, 0x03);

  // Set sample rate = gyroscope output rate/(1 + SMPLRT_DIV)
  writeByte(MPU9250_ADDRESS, MPU9250_SMPLRT_DIV, 0x04);  // Use a 200 Hz rate; a rate consistent with the filter update rate
  // determined inset in CONFIG above

  // Set gyroscope full scale range
  // Range selects FS_SEL and GFS_SEL are 0 - 3, so 2-bit values are left-shifted into positions 4:3
  uint8_t c = readByte(MPU9250_ADDRESS, MPU9250_GYRO_CONFIG);
  //  writeRegister(GYRO_CONFIG, c & ~0xE0); // Clear self-test bits [7:5]
  writeByte(MPU9250_ADDRESS, MPU9250_GYRO_CONFIG, c & ~0x03); // Clear Fchoice bits [1:0]
  writeByte(MPU9250_ADDRESS, MPU9250_GYRO_CONFIG, c & ~0x18); // Clear GFS bits [4:3]
  writeByte(MPU9250_ADDRESS, MPU9250_GYRO_CONFIG, c | MPU9250Gscale << 3); // Set full scale range for the gyro
  // writeRegister(GYRO_CONFIG, c | 0x00); // Set Fchoice for the gyro to 11 by writing its inverse to bits 1:0 of GYRO_CONFIG

  // Set accelerometer full-scale range configuration
  c = readByte(MPU9250_ADDRESS, MPU9250_ACCEL_CONFIG);
  //  writeRegister(ACCEL_CONFIG, c & ~0xE0); // Clear self-test bits [7:5]
  writeByte(MPU9250_ADDRESS, MPU9250_ACCEL_CONFIG, c & ~0x18); // Clear AFS bits [4:3]
  writeByte(MPU9250_ADDRESS, MPU9250_ACCEL_CONFIG, c | MPU9250Ascale << 3); // Set full scale range for the accelerometer

  // Set accelerometer sample rate configuration
  // It is possible to get a 4 kHz sample rate from the accelerometer by choosing 1 for
  // accel_fchoice_b bit [3]; in this case the bandwidth is 1.13 kHz
  c = readByte(MPU9250_ADDRESS, MPU9250_ACCEL_CONFIG2);
  writeByte(MPU9250_ADDRESS, MPU9250_ACCEL_CONFIG2, c & ~0x0F); // Clear accel_fchoice_b (bit 3) and A_DLPFG (bits [2:0])
  writeByte(MPU9250_ADDRESS, MPU9250_ACCEL_CONFIG2, c | 0x03); // Set accelerometer rate to 1 kHz and bandwidth to 41 Hz

  // The accelerometer, gyro, and thermometer are set to 1 kHz sample rates,
  // but all these rates are further reduced by a factor of 5 to 200 Hz because of the SMPLRT_DIV setting

  // Configure Interrupts and Bypass Enable
  // Set interrupt pin active high, push-pull, hold interrupt pin level HIGH until interrupt cleared,
  // clear on read of INT_STATUS, and enable I2C_BYPASS_EN so additional chips
  // can join the I2C bus and all can be controlled by the Arduino as master
  writeByte(MPU9250_ADDRESS, MPU9250_INT_PIN_CFG, 0x22);
  writeByte(MPU9250_ADDRESS, MPU9250_INT_ENABLE, 0x01);  // Enable data ready (bit 0) interrupt
  delay(100);
}


// Function which accumulates gyro and accelerometer data after device initialization. It calculates the average
// of the at-rest readings and then loads the resulting offsets into accelerometer and gyro bias registers.
void accelgyrocalMPU9250(float * dest1, float * dest2) {
  uint8_t data[12]; // data array to hold accelerometer and gyro x, y, z, data
  uint16_t ii, packet_count, fifo_count;
  int32_t gyro_bias[3]  = {0, 0, 0}, accel_bias[3] = {0, 0, 0};

  // reset device
  writeByte(MPU9250_ADDRESS, MPU9250_PWR_MGMT_1, 0x80); // Write a one to bit 7 reset bit; toggle reset device
  delay(100);

  // get stable time source; Auto select clock source to be PLL gyroscope reference if ready
  // else use the internal oscillator, bits 2:0 = 001
  writeByte(MPU9250_ADDRESS, MPU9250_PWR_MGMT_1, 0x01);
  writeByte(MPU9250_ADDRESS, MPU9250_PWR_MGMT_2, 0x00);
  delay(200);

  // Configure device for bias calculation
  writeByte(MPU9250_ADDRESS, MPU9250_INT_ENABLE, 0x00);   // Disable all interrupts
  writeByte(MPU9250_ADDRESS, MPU9250_FIFO_EN, 0x00);      // Disable FIFO
  writeByte(MPU9250_ADDRESS, MPU9250_PWR_MGMT_1, 0x00);   // Turn on internal clock source
  writeByte(MPU9250_ADDRESS, MPU9250_I2C_MST_CTRL, 0x00); // Disable I2C master
  writeByte(MPU9250_ADDRESS, MPU9250_USER_CTRL, 0x00);    // Disable FIFO and I2C master modes
  writeByte(MPU9250_ADDRESS, MPU9250_USER_CTRL, 0x0C);    // Reset FIFO and DMP
  delay(15);

  // Configure MPU6050 gyro and accelerometer for bias calculation
  writeByte(MPU9250_ADDRESS, MPU9250_CONFIG, 0x01);      // Set low-pass filter to 188 Hz
  writeByte(MPU9250_ADDRESS, MPU9250_SMPLRT_DIV, 0x00);  // Set sample rate to 1 kHz
  writeByte(MPU9250_ADDRESS, MPU9250_GYRO_CONFIG, 0x00);  // Set gyro full-scale to 250 degrees per second, maximum sensitivity
  writeByte(MPU9250_ADDRESS, MPU9250_ACCEL_CONFIG, 0x00); // Set accelerometer full-scale to 2 g, maximum sensitivity

  uint16_t  gyrosensitivity  = 131;   // = 131 LSB/degrees/sec
  uint16_t  accelsensitivity = 16384;  // = 16384 LSB/g

  // Configure FIFO to capture accelerometer and gyro data for bias calculation
  writeByte(MPU9250_ADDRESS, MPU9250_USER_CTRL, 0x40);   // Enable FIFO
  writeByte(MPU9250_ADDRESS, MPU9250_FIFO_EN, 0x78);     // Enable gyro and accelerometer sensors for FIFO  (max size 512 bytes in MPU-9150)
  delay(40); // accumulate 40 samples in 40 milliseconds = 480 bytes

  // At end of sample accumulation, turn off FIFO sensor read
  writeByte(MPU9250_ADDRESS, MPU9250_FIFO_EN, 0x00);        // Disable gyro and accelerometer sensors for FIFO
  readBytes(MPU9250_ADDRESS, MPU9250_FIFO_COUNTH, 2, &data[0]); // read FIFO sample count
  fifo_count = ((uint16_t)data[0] << 8) | data[1];
  packet_count = fifo_count / 12; // How many sets of full gyro and accelerometer data for averaging

  for (ii = 0; ii < packet_count; ii++) {
    int16_t accel_temp[3] = {0, 0, 0}, gyro_temp[3] = {0, 0, 0};
    readBytes(MPU9250_ADDRESS, MPU9250_FIFO_R_W, 12, &data[0]); // read data for averaging
    accel_temp[0] = (int16_t) (((int16_t)data[0] << 8) | data[1]  ) ;  // Form signed 16-bit integer for each sample in FIFO
    accel_temp[1] = (int16_t) (((int16_t)data[2] << 8) | data[3]  ) ;
    accel_temp[2] = (int16_t) (((int16_t)data[4] << 8) | data[5]  ) ;
    gyro_temp[0]  = (int16_t) (((int16_t)data[6] << 8) | data[7]  ) ;
    gyro_temp[1]  = (int16_t) (((int16_t)data[8] << 8) | data[9]  ) ;
    gyro_temp[2]  = (int16_t) (((int16_t)data[10] << 8) | data[11]) ;

    accel_bias[0] += (int32_t) accel_temp[0]; // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
    accel_bias[1] += (int32_t) accel_temp[1];
    accel_bias[2] += (int32_t) accel_temp[2];
    gyro_bias[0]  += (int32_t) gyro_temp[0];
    gyro_bias[1]  += (int32_t) gyro_temp[1];
    gyro_bias[2]  += (int32_t) gyro_temp[2];

  }
  accel_bias[0] /= (int32_t) packet_count; // Normalize sums to get average count biases
  accel_bias[1] /= (int32_t) packet_count;
  accel_bias[2] /= (int32_t) packet_count;
  gyro_bias[0]  /= (int32_t) packet_count;
  gyro_bias[1]  /= (int32_t) packet_count;
  gyro_bias[2]  /= (int32_t) packet_count;

  if (accel_bias[2] > 0L) {
    accel_bias[2] -= (int32_t) accelsensitivity; // Remove gravity from the z-axis accelerometer bias calculation
  }
  else {
    accel_bias[2] += (int32_t) accelsensitivity;
  }

  // Construct the gyro biases for push to the hardware gyro bias registers, which are reset to zero upon device startup
  data[0] = (-gyro_bias[0] / 4  >> 8) & 0xFF; // Divide by 4 to get 32.9 LSB per deg/s to conform to expected bias input format
  data[1] = (-gyro_bias[0] / 4)       & 0xFF; // Biases are additive, so change sign on calculated average gyro biases
  data[2] = (-gyro_bias[1] / 4  >> 8) & 0xFF;
  data[3] = (-gyro_bias[1] / 4)       & 0xFF;
  data[4] = (-gyro_bias[2] / 4  >> 8) & 0xFF;
  data[5] = (-gyro_bias[2] / 4)       & 0xFF;

  // Push gyro biases to hardware registers
  writeByte(MPU9250_ADDRESS, MPU9250_XG_OFFSET_H, data[0]);
  writeByte(MPU9250_ADDRESS, MPU9250_XG_OFFSET_L, data[1]);
  writeByte(MPU9250_ADDRESS, MPU9250_YG_OFFSET_H, data[2]);
  writeByte(MPU9250_ADDRESS, MPU9250_YG_OFFSET_L, data[3]);
  writeByte(MPU9250_ADDRESS, MPU9250_ZG_OFFSET_H, data[4]);
  writeByte(MPU9250_ADDRESS, MPU9250_ZG_OFFSET_L, data[5]);

  // Output scaled gyro biases for display in the main program
  dest1[0] = (float) gyro_bias[0] / (float) gyrosensitivity;
  dest1[1] = (float) gyro_bias[1] / (float) gyrosensitivity;
  dest1[2] = (float) gyro_bias[2] / (float) gyrosensitivity;

  // Construct the accelerometer biases for push to the hardware accelerometer bias registers. These registers contain
  // factory trim values which must be added to the calculated accelerometer biases; on boot up these registers will hold
  // non-zero values. In addition, bit 0 of the lower byte must be preserved since it is used for temperature
  // compensation calculations. Accelerometer bias registers expect bias input as 2048 LSB per g, so that
  // the accelerometer biases calculated above must be divided by 8.

  int32_t accel_bias_reg[3] = {0, 0, 0}; // A place to hold the factory accelerometer trim biases
  readBytes(MPU9250_ADDRESS, MPU9250_XA_OFFSET_H, 2, &data[0]); // Read factory accelerometer trim values
  accel_bias_reg[0] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
  readBytes(MPU9250_ADDRESS, MPU9250_YA_OFFSET_H, 2, &data[0]);
  accel_bias_reg[1] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
  readBytes(MPU9250_ADDRESS, MPU9250_ZA_OFFSET_H, 2, &data[0]);
  accel_bias_reg[2] = (int32_t) (((int16_t)data[0] << 8) | data[1]);

  uint32_t mask = 1uL; // Define mask for temperature compensation bit 0 of lower byte of accelerometer bias registers
  uint8_t mask_bit[3] = {0, 0, 0}; // Define array to hold mask bit for each accelerometer bias axis

  for (ii = 0; ii < 3; ii++) {
    if ((accel_bias_reg[ii] & mask)) mask_bit[ii] = 0x01; // If temperature compensation bit is set, record that fact in mask_bit
  }

  // Construct total accelerometer bias, including calculated average accelerometer bias from above
  accel_bias_reg[0] -= (accel_bias[0] / 8); // Subtract calculated averaged accelerometer bias scaled to 2048 LSB/g (16 g full scale)
  accel_bias_reg[1] -= (accel_bias[1] / 8);
  accel_bias_reg[2] -= (accel_bias[2] / 8);

  data[0] = (accel_bias_reg[0] >> 8) & 0xFF;
  data[1] = (accel_bias_reg[0])      & 0xFF;
  data[1] = data[1] | mask_bit[0]; // preserve temperature compensation bit when writing back to accelerometer bias registers
  data[2] = (accel_bias_reg[1] >> 8) & 0xFF;
  data[3] = (accel_bias_reg[1])      & 0xFF;
  data[3] = data[3] | mask_bit[1]; // preserve temperature compensation bit when writing back to accelerometer bias registers
  data[4] = (accel_bias_reg[2] >> 8) & 0xFF;
  data[5] = (accel_bias_reg[2])      & 0xFF;
  data[5] = data[5] | mask_bit[2]; // preserve temperature compensation bit when writing back to accelerometer bias registers

  // Apparently this is not working for the acceleration biases in the MPU-9250
  // Are we handling the temperature correction bit properly?
  // Push accelerometer biases to hardware registers
  /*  writeByte(MPU9250_ADDRESS, XA_OFFSET_H, data[0]);
    writeByte(MPU9250_ADDRESS, XA_OFFSET_L, data[1]);
    writeByte(MPU9250_ADDRESS, YA_OFFSET_H, data[2]);
    writeByte(MPU9250_ADDRESS, YA_OFFSET_L, data[3]);
    writeByte(MPU9250_ADDRESS, ZA_OFFSET_H, data[4]);
    writeByte(MPU9250_ADDRESS, ZA_OFFSET_L, data[5]);
  */
  // Output scaled accelerometer biases for display in the main program
  dest2[0] = (float)accel_bias[0] / (float)accelsensitivity;
  dest2[1] = (float)accel_bias[1] / (float)accelsensitivity;
  dest2[2] = (float)accel_bias[2] / (float)accelsensitivity;
}


void magcalMPU9250(float * dest1) {
  uint16_t ii = 0, sample_count = 0;
  int32_t mag_bias[3] = {0, 0, 0};
  int16_t mag_max[3] = { -32767, -32767, -32767}, mag_min[3] = {32767, 32767, 32767}, mag_temp[3] = {0, 0, 0};

  Serial.println("Mag Calibration: Wave device in a figure eight until done!");
  delay(4000);

  sample_count = 64;
  for (ii = 0; ii < sample_count; ii++) {
    MPU9250readMagData(mag_temp);  // Read the mag data
    for (int jj = 0; jj < 3; jj++) {
      if (mag_temp[jj] > mag_max[jj]) mag_max[jj] = mag_temp[jj];
      if (mag_temp[jj] < mag_min[jj]) mag_min[jj] = mag_temp[jj];
    }
    delay(135);  // at 8 Hz ODR, new mag data is available every 125 ms
  }

  //    Serial.println("mag x min/max:"); Serial.println(mag_max[0]); Serial.println(mag_min[0]);
  //    Serial.println("mag y min/max:"); Serial.println(mag_max[1]); Serial.println(mag_min[1]);
  //    Serial.println("mag z min/max:"); Serial.println(mag_max[2]); Serial.println(mag_min[2]);

  mag_bias[0]  = (mag_max[0] + mag_min[0]) / 2; // get average x mag bias in counts
  mag_bias[1]  = (mag_max[1] + mag_min[1]) / 2; // get average y mag bias in counts
  mag_bias[2]  = (mag_max[2] + mag_min[2]) / 2; // get average z mag bias in counts

  dest1[0] = (float) mag_bias[0] * MPU9250mRes * magCalibration[0]; // save mag biases in G for main program
  dest1[1] = (float) mag_bias[1] * MPU9250mRes * magCalibration[1];
  dest1[2] = (float) mag_bias[2] * MPU9250mRes * magCalibration[2];

  Serial.println("Mag Calibration done!");
}


// Accelerometer and gyroscope self test; check calibration wrt factory settings
// Should return percent deviation from factory trim values, +/- 14 or less deviation is a pass
void MPU9250SelfTest(float * destination) {
  uint8_t rawData[6] = {0, 0, 0, 0, 0, 0};
  uint8_t selfTest[6];
  int32_t gAvg[3] = {0}, aAvg[3] = {0}, aSTAvg[3] = {0}, gSTAvg[3] = {0};
  float factoryTrim[6];
  uint8_t FS = 0;

  writeByte(MPU9250_ADDRESS, MPU9250_SMPLRT_DIV, 0x00);    // Set gyro sample rate to 1 kHz
  writeByte(MPU9250_ADDRESS, MPU9250_CONFIG, 0x02);        // Set gyro sample rate to 1 kHz and DLPF to 92 Hz
  writeByte(MPU9250_ADDRESS, MPU9250_GYRO_CONFIG, 1 << FS); // Set full scale range for the gyro to 250 dps
  writeByte(MPU9250_ADDRESS, MPU9250_ACCEL_CONFIG2, 0x02); // Set accelerometer rate to 1 kHz and bandwidth to 92 Hz
  writeByte(MPU9250_ADDRESS, MPU9250_ACCEL_CONFIG, 1 << FS); // Set full scale range for the accelerometer to 2 g

  for ( int ii = 0; ii < 200; ii++) { // get average current values of gyro and acclerometer

    readBytes(MPU9250_ADDRESS, MPU9250_ACCEL_XOUT_H, 6, &rawData[0]);        // Read the six raw data registers into data array
    aAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
    aAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
    aAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;

    readBytes(MPU9250_ADDRESS, MPU9250_GYRO_XOUT_H, 6, &rawData[0]);       // Read the six raw data registers sequentially into data array
    gAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
    gAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
    gAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;
  }

  for (int ii = 0; ii < 3; ii++) { // Get average of 200 values and store as average current readings
    aAvg[ii] /= 200;
    gAvg[ii] /= 200;
  }

  // Configure the accelerometer for self-test
  writeByte(MPU9250_ADDRESS, MPU9250_ACCEL_CONFIG, 0xE0); // Enable self test on all three axes and set accelerometer range to +/- 2 g
  writeByte(MPU9250_ADDRESS, MPU9250_GYRO_CONFIG,  0xE0); // Enable self test on all three axes and set gyro range to +/- 250 degrees/s
  delay(25);  // Delay a while to let the device stabilize

  for ( uint16_t ii = 0; ii < 200; ii++) { // get average self-test values of gyro and acclerometer

    readBytes(MPU9250_ADDRESS, MPU9250_ACCEL_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers into data array
    aSTAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
    aSTAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
    aSTAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;

    readBytes(MPU9250_ADDRESS, MPU9250_GYRO_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers sequentially into data array
    gSTAvg[0] += (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
    gSTAvg[1] += (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
    gSTAvg[2] += (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;
  }

  for (int ii = 0; ii < 3; ii++) { // Get average of 200 values and store as average self-test readings
    aSTAvg[ii] /= 200;
    gSTAvg[ii] /= 200;
  }

  // Configure the gyro and accelerometer for normal operation
  writeByte(MPU9250_ADDRESS, MPU9250_ACCEL_CONFIG, 0x00);
  writeByte(MPU9250_ADDRESS, MPU9250_GYRO_CONFIG,  0x00);
  delay(25);  // Delay a while to let the device stabilize

  // Retrieve accelerometer and gyro factory Self-Test Code from USR_Reg
  selfTest[0] = readByte(MPU9250_ADDRESS, MPU9250_SELF_TEST_X_ACCEL); // X-axis accel self-test results
  selfTest[1] = readByte(MPU9250_ADDRESS, MPU9250_SELF_TEST_Y_ACCEL); // Y-axis accel self-test results
  selfTest[2] = readByte(MPU9250_ADDRESS, MPU9250_SELF_TEST_Z_ACCEL); // Z-axis accel self-test results
  selfTest[3] = readByte(MPU9250_ADDRESS, MPU9250_SELF_TEST_X_GYRO);  // X-axis gyro self-test results
  selfTest[4] = readByte(MPU9250_ADDRESS, MPU9250_SELF_TEST_Y_GYRO);  // Y-axis gyro self-test results
  selfTest[5] = readByte(MPU9250_ADDRESS, MPU9250_SELF_TEST_Z_GYRO);  // Z-axis gyro self-test results

  // Retrieve factory self-test value from self-test code reads
  factoryTrim[0] = (float)(2620 / 1 << FS) * (pow( 1.01 , ((float)selfTest[0] - 1.0) )); // FT[Xa] factory trim calculation
  factoryTrim[1] = (float)(2620 / 1 << FS) * (pow( 1.01 , ((float)selfTest[1] - 1.0) )); // FT[Ya] factory trim calculation
  factoryTrim[2] = (float)(2620 / 1 << FS) * (pow( 1.01 , ((float)selfTest[2] - 1.0) )); // FT[Za] factory trim calculation
  factoryTrim[3] = (float)(2620 / 1 << FS) * (pow( 1.01 , ((float)selfTest[3] - 1.0) )); // FT[Xg] factory trim calculation
  factoryTrim[4] = (float)(2620 / 1 << FS) * (pow( 1.01 , ((float)selfTest[4] - 1.0) )); // FT[Yg] factory trim calculation
  factoryTrim[5] = (float)(2620 / 1 << FS) * (pow( 1.01 , ((float)selfTest[5] - 1.0) )); // FT[Zg] factory trim calculation

  // Report results as a ratio of (STR - FT)/FT; the change from Factory Trim of the Self-Test Response
  // To get percent, must multiply by 100
  for (int i = 0; i < 3; i++) {
    destination[i]   = 100.0 * ((float)(aSTAvg[i] - aAvg[i])) / factoryTrim[i] - 100.; // Report percent differences
    destination[i + 3] = 100.0 * ((float)(gSTAvg[i] - gAvg[i])) / factoryTrim[i + 3] - 100.; // Report percent differences
  }
}



// I2C scan function

void I2Cscan() {
  // scan for i2c devices
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


// I2C read/write functions for the MPU9250 and AK8963 sensors

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


