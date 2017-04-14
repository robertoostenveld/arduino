// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#include "payload.h"
extern payload_t payload;

unsigned long lastDisplay = 0;


uint32_t readVcc();
unsigned long crc_buf(char *b, unsigned long l);

// Data wire is plugged into port 15 on the Teensy
#define ONE_WIRE_BUS 15

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature dallasTemperature(&oneWire);

// TinyGPS++ requires the use of SoftwareSerial, and assumes that you have a
// 4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
// SoftwareSerial ss(RXPin, TXPin);
#define ss Serial1


/*****************************************************************************/

void init_sensor() {
  // Start up the library
  dallasTemperature.begin();

  analogReference(EXTERNAL);
  analogReadResolution(12);
  analogReadAveraging(32); // this one is optional.

  ss.begin(GPSBaud);
}

/*****************************************************************************/

void read_sensor() {
  float lat = 0, lng = 0, alt = 0, temp = 0, volt = 0;
  int verbose = 1;

  volt = 0.001 * readVcc(); // this is in mV, we want V

  dallasTemperature.requestTemperatures();
  temp = dallasTemperature.getTempCByIndex(0);

  unsigned long timeout = millis() + 1000;
  while (millis() < timeout) {
    // keep reading until a full sentence is correctly encoded
    if (ss.available() > 0) {
      if (gps.encode(ss.read()) || millis() > timeout) {
        timeout = millis();
        if (gps.location.isValid()) {
          lat = gps.location.lat();
          lng = gps.location.lng();
          alt = gps.altitude.meters();
        }
      }
    }
    else {
      // wait for some new data to come in from the GPS
      delay(10);
    }
  }

  payload.value1      = volt;
  payload.value2      = temp;
  payload.value3      = lat;
  payload.value4      = lng;
  payload.value5      = alt;
  payload.counter    += 1;
  payload.crc         = crc_buf((char *)&payload, sizeof(payload_t) - sizeof(unsigned long));

  if (verbose) {
    Serial.print(payload.id);
    Serial.print(",\t");
    Serial.print(payload.counter);
    Serial.print(",\t");
    Serial.print(payload.value1, 2);
    Serial.print(",\t");
    Serial.print(payload.value2, 2);
    Serial.print(",\t");
    Serial.print(payload.value3, 6);
    Serial.print(",\t");
    Serial.print(payload.value4, 6);
    Serial.print(",\t");
    Serial.print(payload.value5, 2);
    Serial.print(",\t");
    Serial.println(payload.crc);
  }
}

/*****************************************************************************/

// see https://forum.pjrc.com/threads/26117-Teensy-3-1-Voltage-sensing-and-low-battery-alert
// for Teensy 3.0, only valid between 2.0V and 3.5V. Returns in millivolts.
uint32_t readVcc() {
  uint32_t x = analogRead(39);
  return (178 * x * x + 2688757565 - 1184375 * x) / 372346;
};

/*****************************************************************************/

static PROGMEM prog_uint32_t crc_table[16] = {
  0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
  0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
  0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
  0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

/*****************************************************************************/

unsigned long crc_update(unsigned long crc, byte data)
{
  byte tbl_NODEx;
  tbl_NODEx = crc ^ (data >> (0 * 4));
  crc = pgm_read_dword_near(crc_table + (tbl_NODEx & 0x0f)) ^ (crc >> 4);
  tbl_NODEx = crc ^ (data >> (1 * 4));
  crc = pgm_read_dword_near(crc_table + (tbl_NODEx & 0x0f)) ^ (crc >> 4);
  return crc;
}

/*****************************************************************************/

unsigned long crc_buf(char *b, unsigned long l)
{
  unsigned long crc = ~0L;
  for (unsigned long i = 0; i < l; i++)
    crc = crc_update(crc, ((char *)b)[i]);
  crc = ~crc;
  return crc;
}

/*****************************************************************************/

unsigned long crc_string(char *s)
{
  unsigned long crc = ~0L;
  while (*s)
    crc = crc_update(crc, *s++);
  crc = ~crc;
  return crc;
}

