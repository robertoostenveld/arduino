/* 
 * Sketch for an Arduino Uno or Leonardo connected to two Adafruit DAC7578 modules. The purpose is to 
 * act as a MEG phantom driver, controlling a small current through up to 16 magnetic dipole or 
 * equivalent current dipole sources, thereby generating small magnetic fields that can be recorded 
 * using an OPM- or SQUID-based MEG system.
 *
 * See 
 *   https://store.arduino.cc/products/arduino-uno-rev3-smd
 *   https://store.arduino.cc/products/arduino-leonardo-with-headers
 *   https://learn.adafruit.com/adafruit-dac7578-8-x-channel-12-bit-i2c-dac
 */

#include <Adafruit_DACX578.h>
#include <Adafruit_I2CDevice.h>
#include <math.h>

#include "blink_led.h"

Adafruit_DACX578 dac0(12);
Adafruit_DACX578 dac1(12);
Adafruit_DACX578 dac2(12);

const uint8_t nchannel = 24;

const uint8_t addr0 = 0x4C;  // open pads
const uint8_t addr1 = 0x48;  // left pads
const uint8_t addr2 = 0x4A;  // right pads

float frequency[24] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 };
float phase[24] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const float freqshift = 0;

const uint16_t amplitude = 2048;   // Half of full scale for 12-bit dac0 (0 to 4095)
const uint16_t offset = 2048;      // DC offset to keep sine wave positive
const uint32_t sampleTime = 1000;  // Approximate time between samples, in microseconds
const uint32_t feedbackTime = -1;  // Time between serial feedback, in microseconds

void retry(uint32_t duration) {
  // wait for some time, keep the LED blinking
  uint32_t start = millis();
  while ((millis() - start) < duration) {
    blinker.update();
    delay(10);
  }
};

void setup() {
  // blink rapidly, this will continue indefinitely when the setup fails
  ledInit();
  ledFast();

  Serial.begin(115200);
  while (!Serial && (millis() < 5000)) {
    // wait for the serial interface to start, but not longer than 5 seconds
    retry(100);
  }

  Serial.println("-------------------------------");
  Serial.println("Arduino dac7578 sine wave generator");
  Serial.print("firmware ");
  Serial.print(__TIME__);
  Serial.print(" / ");
  Serial.println(__DATE__);

  if (nchannel > 0) {
    // initialize the first DACX578
    while (!dac0.begin(addr0, &Wire)) {
      Serial.println("Failed to find dac0");
      retry(1000);
    }
    Serial.println("dac0 initialized");
  }

  if (nchannel > 8) {
    // initialize the second DACX578
    while (!dac1.begin(addr1, &Wire)) {
      Serial.println("Failed to find dac1");
      retry(1000);
    }
    Serial.println("dac1 initialized");
  }

  if (nchannel > 16) {
    // initialize the third DACX578
    while (!dac2.begin(addr2, &Wire)) {
      Serial.println("Failed to find dac2");
      retry(1000);
    }
    Serial.println("dac2 initialized");
  }

  for (uint8_t channel = 0; channel < nchannel; channel++) {
    // shift the frequency
    frequency[channel] += freqshift;
    Serial.print("channel ");
    Serial.print(channel + 1);
    Serial.print(" = ");
    Serial.print(frequency[channel]);
    Serial.println(" Hz");
  }

  // set I2C frequency to 3.4 MHz for faster communication
  Wire.setClock(3400000);

  // blink slowly for the rest of the time
  ledSlow();
}

void loop() {
  static uint32_t lastSample = micros();
  static uint32_t lastFeedback = micros();

  uint32_t currentTime = micros();
  uint32_t deltaTime = currentTime - lastSample;

  // this is needed to keep the LED blinking
  blinker.update();

  if (deltaTime >= sampleTime) {

    for (uint8_t channel = 0; channel < nchannel; channel++) {
      // increment the phase according to the channels-specific frequency
      phase[channel] += 2.0 * M_PI * frequency[channel] * ((float)deltaTime / 1000000);
      if (phase[channel] >= 2.0 * M_PI) {
        phase[channel] -= 2.0 * M_PI;
      }

      // compute the new value
      float sineValue = sin(phase[channel]) * amplitude + offset;

      // set the output to the new value
      if (channel < 8)
        // channels 0-7 on the first DAC
        dac0.writeAndUpdateChannelValue(channel, (uint16_t)sineValue);
      else if (channel < 16)
        // channels 8-15 correspond to channels 0-7 on the second DAC
        dac1.writeAndUpdateChannelValue(channel - 8, (uint16_t)sineValue);
      else if (channel < 24)
        // channels 16-23 correspond to channels 0-7 on the third DAC
        dac2.writeAndUpdateChannelValue(channel - 16, (uint16_t)sineValue);
    }
    lastSample = currentTime;

    if ((currentTime - lastFeedback) > feedbackTime) {
      Serial.println(deltaTime);
      lastFeedback = currentTime;
    }

  }  // if deltaTime

}  // loop
