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

## Hardware connections

The ESP8266 is connected to a TCA9548a I2C multiplexer as follows:

- SDA=D1=5
- SCL=D2=4
- RESET=D3=0
- VCC
- GND

The multiplexer is in turn connected to eight MPU9250 sensors with the I2C connections:

- SDA
- SCL
- VCC
- GND

Furthermore, the ESP8266 is connected (over some resistors) to a common cathode RGB LED as follows:

- LED_R=D5
- LED_G=D6
- LED_B=D7
- GND

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

The device has a RGB LED to indicate the status:
  * yellow - initial configuration (should be short)
  * red - failed to connect to WiFi network, access point started
  * constant green - setting up all sensors (should be relatively short)
  * blinking green - measuring and transmitting data
  * blue - recent activity on the web interface (for 5 seconds)

## Decimation

The device will sample as fast as possible and send all data in UDP
packets. The amount of network traffic can get very large and in
my experience this can clog up your network. You can specify a
decimation factor, which specifies that only every Nth measurement
is to be transmitted.

# SPIFFS for static files

Please note that you should not only write the firmware to the ESP8266 module, but also the static content for the web interface. The html, css and javascript files located in the data directory should be written to the SPIFS filesystem on the ESP8266. See for example http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html and https://www.instructables.com/id/Using-ESP8266-SPIFFS for instructions.
You will get a "file not found" error if the firmware cannot access the data files.

## Known issues

The OSC message gets too large for an UDP packet with more than 6
sensorrs AND with the AHRS switched on. The consequence is that no
data gets transmitted. So for 7 or 8 sensors you should keep AHRS
switched off.

## References

You can find the conceptual design and some photos on my [home page](http://robertoostenveld.nl/?p=835&preview=true).

The IMU and AHRS code is mostly based on https://github.com/kriswiner/MPU9250.

The interface for the wifi configuration is based on https://github.com/tzapu/WiFiManager.

The [OSCulator](https://osculator.net) software is convenient for debugging.

This motion capture device was designed for the [EEGsynth](http://eegsynth.org).
