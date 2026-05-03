/* 
 * Sketch for an Arduino Uno or Leonardo connected to two Adafruit DAC7578 modules. The purpose is to 
 * act as a MEG phantom driver, controlling a small current through up to 16 magnetic dipole or 
 * equivalent current dipole sources, thereby generating small magnetic fields that can be recorded 
 * using an OPM or SQUID-based MEG system.
 *
 * See 
 *   https://store.arduino.cc/products/arduino-uno-rev3-smd
 *   https://store.arduino.cc/products/arduino-leonardo-with-headers
 *   https://learn.adafruit.com/adafruit-dac7578-8-x-channel-12-bit-i2c-dac
 */

#define USE_LOOKUP
#define M_2PI 6.28318530717958647692528676655900576

#include <Adafruit_DACX578.h>
#include <Adafruit_I2CDevice.h>

#include "blink_led.h"

#ifdef USE_LOOKUP
#include "lookup.h"
#else
#include <math.h>
#endif

Adafruit_DACX578 dac0(12);
Adafruit_DACX578 dac1(12);
Adafruit_DACX578 dac2(12);

bool dac0Found = false;
bool dac1Found = false;
bool dac2Found = false;

const uint8_t addr0 = 0x4C;  // open pads
const uint8_t addr1 = 0x48;  // left pads
const uint8_t addr2 = 0x4A;  // right pads

// these define the sine waves
const float freqoffset = 5;            // Frequency for the first channel
const float freqstepsize = 0.2;        // Frequency spacing between channels
const float offset = 2048;             // Offset for the 12-bit DAC output (0 to 4095)
const float amplitude = 2047;          // Half of the full scale for 12-bit DAC output (0 to 4095)
const uint32_t sampleTime = 5000;      // Approximate time between samples, in microseconds
const uint32_t feedbackTime = -1;      // Time between serial feedback, in microseconds (-1 for no feedback)

// these will be updated according to the number of DACs found
uint8_t nchannels = 24;
uint8_t dacBoard[24] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2 };
uint8_t dacChannel[24] = { 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7 };
float frequency[24] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
float phase[24] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void blinkdelay(uint32_t duration) {
  // wait for some time, but keep the LED blinking
  uint32_t start = millis();
  while ((millis() - start) < duration) {
    blink.update();
    delay(10);
  }
};

void setup() {
  // allow some time for reprogramming
  delay(2000);

  // blink rapidly, this will continue indefinitely when the setup fails
  ledInit();
  ledFast();

  Serial.begin(115200);
  while (!Serial && (millis() < 3000)) {
    // wait for the serial interface to start, but not longer than 3 seconds
    blinkdelay(200);
  }

  Serial.println("-------------------------------");
  Serial.println("Arduino dac7578 sine wave generator");
  Serial.print("Firmware version ");
  Serial.print(__TIME__);
  Serial.print(" / ");
  Serial.println(__DATE__);

  // initialize the first DAC7578
  dac0Found = dac0.begin(addr0, &Wire);
  long initstart = millis();
  while (!dac0Found && (millis() - initstart < 2000)) {
    blinkdelay(200);
    dac0Found = dac0.begin(addr0, &Wire);
  }
  if (!dac0Found) {
    Serial.print("Failed to find dac0");
  } else {
    Serial.print("Initialized dac0");
  }
  Serial.print(" at address ");
  Serial.println(addr0, HEX);

  // initialize the second DACC7578
  dac1Found = dac1.begin(addr1, &Wire);
  initstart = millis();
  while (!dac1Found && (millis() - initstart < 2000)) {
    blinkdelay(200);
    dac1Found = dac1.begin(addr1, &Wire);
  }
  if (!dac1Found) {
    Serial.print("Failed to find dac1");
  } else {
    Serial.print("Initialized dac1");
  }
  Serial.print(" at address ");
  Serial.println(addr1, HEX);

  // initialize the third DACC7578
  dac2Found = dac2.begin(addr2, &Wire);
  initstart = millis();
  while (!dac2Found && (millis() - initstart < 2000)) {
    blinkdelay(200);
    dac2Found = dac2.begin(addr2, &Wire);
  }
  if (!dac2Found) {
    Serial.print("Failed to find dac2");
  } else {
    Serial.print("Initialized dac2");
  }
  Serial.print(" at address ");
  Serial.println(addr2, HEX);

  // update the channel-to-board mapping
  for (uint8_t channel = 0; channel < 24; channel++) {
    // if board 0 is not found, all its channels shift to the next board
    if (!dac0Found && dacBoard[channel] >= 0) {
      dacBoard[channel]++;
    }
    // if board 1 is not found, all its channels shift to the next board
    if (!dac1Found && dacBoard[channel] >= 1) {
      dacBoard[channel]++;
    }
    // if board 1 is not found, all its channels shift to the next board
    if (!dac2Found && dacBoard[channel] >= 2) {
      dacBoard[channel]++;
    }
  }

  // update the number of channels according to the number of DAC boards that were found
  nchannels = 8 * (dac0Found + dac1Found + dac2Found);

  // set the frequency for each channel
  for (uint8_t channel = 0; channel < nchannels; channel++) {
    frequency[channel] = freqoffset + freqstepsize * channel;
    Serial.print("channel ");
    Serial.print(channel + 1);
    Serial.print(", board ");
    Serial.print(dacBoard[channel] + 1);
    Serial.print(", frequency ");
    Serial.print(frequency[channel]);
    Serial.println(" Hz");
  }

#ifdef USE_LOOKUP
  // apply the offset and amplitude to the sine wave lookup table
  for (uint16_t index = 0; index < LOOKUP_LEN; index++) {
    float value = lookup_value[index];
    // the lookup table is constructed with an offset of 2048 and an amplitude of 2047, for a total range of 0 to 4095
    value -= 2048;
    value /= 2047;
    // apply the user-specified amplitude and offset
    value *= amplitude;
    value += offset;
    lookup_value[index] = value;
  }
#endif

  // blink slowly for the rest of the time
  if (nchannels > 0) {
    ledSlow();
  }

  // The Arduino Uno and Leonardo default to an I2C clock speed of 100 kHz but support a maximum clock speed of 400 kHz
  Wire.setClock(400000);

}  // setup

void loop() {
  static uint32_t lastSample = micros();
  static uint32_t lastFeedback = micros();

  // this is needed to keep the LED blinking
  blink.update();

  uint32_t currentTime = micros();
  uint32_t deltaTime = currentTime - lastSample;

  if (deltaTime >= sampleTime) {
    // compute the time increments in radians
    float deltaTimeRad = deltaTime * (M_2PI / 1000000);

    for (uint8_t channel = 0; channel < nchannels; channel++) {
      // increment the phase according to the channels-specific frequency
      phase[channel] += frequency[channel] * deltaTimeRad;

#ifdef USE_LOOKUP
      // lookup the new value, the lookup table has already been offset and scaled with the amplitude
      uint16_t value = lookup_nearest(phase[channel]);
#else
      // compute the new value
      uint16_t value = (float)(offset + amplitude * sin(phase[channel]));
#endif

      if (channel == -1)
        Serial.println(value);

      // write the value to the corresponding channel of the corresponding dacBoard
      if (dacBoard[channel] == 0) {
        dac0.writeAndUpdateChannelValue(dacChannel[channel], value);
      } else if (dacBoard[channel] == 1) {
        dac1.writeAndUpdateChannelValue(dacChannel[channel], value);
      } else if (dacBoard[channel] == 2) {
        dac2.writeAndUpdateChannelValue(dacChannel[channel], value);
      }
    }
    lastSample = currentTime;

    if ((currentTime - lastFeedback) > feedbackTime) {
      Serial.println(deltaTime);
      lastFeedback = currentTime;
    }

  }  // if deltaTime
}  // loop
