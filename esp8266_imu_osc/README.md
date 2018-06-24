# ESP8266_IMU_OSC

This is a sketch for an 8-channel motion capture system that is
based around a Wemos D1 Mini ESP8622 board, in combination with one
or multiple MPU9250 or MPU9255 inertial motion units. The IMUs are
connected over a TCA9548 I2C multiplex board.

When the device cannot connect to a WiFi network, it will start an
access point with the SSID "IMU-OSC" and the led will turn red.
Connecting to the IMU-OSC network allows you to configure your local
WIFI network name and password.

The data from the IMU sensors is streamed over the WiFi network
using the Open Sound Control (OSC) format.

The sampling rate that can be achieved with one sensor is around
200 Hz, the sampling rate for 8 sensors is around 60 Hz.

## Configuration

Once connected to your local network, you can configure some settings
(see below) by connecting with your browser to http://imu-osc.local
or (for Windows) to the IP address of the device.

The following settings can be configured
  * number of IMU sensors, the maxiumum is 8 (number)
  * decimation factor (number, see below)
  * whether pitch, roll and yaw are to be computed in the AHRS reference frame (boolean)
  * destimation IP address (xxx.xxx.xxx.xxx)
  * destination port (number, the default for OSC is to listen on 8000)

## Status LED

The device has a RGB led to indicate the status:
  * yellow - initial configuration (should be short)
  * red - failed to connect to WiFi network, access point started
  * constant green - setting up all sensors (should be relatively short)
  * blinking green - measuring and transmitting data
  * blue - recent activity on the web interface (for 5 seconds)

## Decimation

The device will sample as fast as possible and send all data in UDP
packets. The amount of network trafick can get very large and in
my experience this can clog up your network. You can specify a
decimation factor, which specifies that only every Nth measurement
is to be transmitted.

## Known issues

The OSC message gets too large for an UDP packet with more than 6
sensorrs AND with the AHRS switched on. The consequence is that no
data gets transmitted. So for 7 or 8 sensors you should keep AHRS
switched off.

## References

The IMU and AHRS code is mostly based on https://github.com/kriswiner/MPU9250.

The interface for the wifi configuration is based on https://github.com/tzapu/WiFiManager.

The [OSCulator](https://osculator.net) software is convenient for debugging.

This motion capture device was designed for the [EEGsynth](http://eegsynth.org).

