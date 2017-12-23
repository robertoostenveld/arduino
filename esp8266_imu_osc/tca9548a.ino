#include "tca9548a.h"

tca9548a::tca9548a(void) {
  _address = TCA9548A_ADDRESS;
}

tca9548a::tca9548a(uint8_t address) {
  _address = address;
}

tca9548a::~tca9548a(void) {
}

void tca9548a::select(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(_address);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

void tca9548a::disable(void) {
  Wire.beginTransmission(_address);
  Wire.write(0);  // no channel selected
  Wire.endTransmission();
}

