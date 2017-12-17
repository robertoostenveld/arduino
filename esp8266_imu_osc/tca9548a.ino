#include "tca9548a.h"

void tcadisable() {
  Wire.beginTransmission(TCA9548A_ADDRESS);
  Wire.write(0);  // no channel selected
  Wire.endTransmission();
}

void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCA9548A_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

