/* 
 * Sketch for an Adafruit RP2040 feather connected to two Adafruit DAC7578 modules. The purpose is to 
 * act as a MEG phantom driver, controlling a small current through up to 16 magnetic dipole or 
 * equivalent current dipole sources, thereby generating small magnetic fields that can be recorded 
 * using an OPM- or SQUID-based MEG system.
 *
 * See 
 *   https://learn.adafruit.com/adafruit-feather-rp2040-pico
 *   https://learn.adafruit.com/adafruit-dac7578-8-x-channel-12-bit-i2c-dac
 */

#include <Adafruit_DACX578.h>
#include <Adafruit_I2CDevice.h>
#include <math.h>

#include "blink_led.h"

Adafruit_DACX578 dac0(12);
Adafruit_DACX578 dac1(12);

const float frequency[16] = { 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36 };  // Hz
const uint16_t amplitude = 2048;                                                         // Half of full scale for 12-bit dac0 (0 to 4095)
const uint16_t offset = 2048;                                                            // DC offset to keep sine wave positive
const uint32_t sampleTime = 1000;                                                        // Approximate time between samples, in microseconds
const uint32_t feedbackTime = -1;                                                        // Time between serial feedback, in microseconds

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println("------------------------------");
  Serial.println("16 channel sine wave generator");

  ledInit();

  // blink quickly for the first second, this will continue indefinitely when the setup fails
  ledFast();
  delay(1000);

  // initialize the first DACX578
  while (!dac0.begin((uint8_t)0x47, &Wire)) {
    Serial.println("Failed to find dac0");
    delay(1000);
  }
  Serial.println("dac0 initialized");

  // initialize the second DACX578
  while (!dac1.begin((uint8_t)0x4C, &Wire)) {
    Serial.println("Failed to find dac1");
    delay(1000);
  }
  Serial.println("dac1 initialized");

  for (uint8_t channel = 0; channel < 16; channel++) {
    Serial.print("channel ");
    Serial.print(channel + 1);
    Serial.print(" = ");
    Serial.print(frequency[channel]);
    Serial.println(" Hz");
  }

  // set I2C frequency to 3.4 MHz for faster communication
  Wire.setClock(3400000);

  // blink slowly, this will continue for the rest of the time
  ledSlow();
}

void loop() {
  static float phase[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  static uint32_t lastSample = micros();
  static uint32_t lastFeedback = micros();

  uint32_t currentTime = micros();
  uint32_t deltaTime = currentTime - lastSample;

  if (deltaTime >= sampleTime) {

    // update the phase for each of the channels
    for (uint8_t channel = 0; channel < 16; channel++) {
      phase[channel] += 2 * M_PI * frequency[channel] * ((float)deltaTime / 1000000);
      if (phase[channel] >= 2 * M_PI) {
        phase[channel] -= 2 * M_PI;
      }

      // compute and set the new value for each of the channels
      float sineValue = sin(phase[channel]) * amplitude + offset;
      if (channel < 8) {
        // channels 0-7 on the first DAC
        dac0.writeAndUpdateChannelValue(channel, (uint16_t)sineValue);
      } else {
        // channels 8-15 correspond to channels 0-7 on the second DAC
        dac1.writeAndUpdateChannelValue(channel-8, (uint16_t)sineValue);
      }
    }
    lastSample = currentTime;

    if ((currentTime - lastFeedback) > feedbackTime) {
      Serial.println(deltaTime);
      lastFeedback = currentTime;
    }

  }  // if deltaTime

}  // loop
