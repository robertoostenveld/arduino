#ifndef _UTIL_H_
#define _UTIL_H_

void print_binary(uint32_t val) {
  for (int l = 31; l >= 24; l--) {
    uint8_t b = (val >> l) & 0x01;
    Serial.print(b);
  }
  Serial.print(' ');
  for (int l = 23; l >= 16; l--) {
    uint8_t b = (val >> l) & 0x01;
    Serial.print(b);
  }
  Serial.print(' ');
  for (int l = 15; l >= 8; l--) {
    uint8_t b = (val >> l) & 0x01;
    Serial.print(b);
  }
  Serial.print(' ');
  for (int l = 7; l >= 0; l--) {
    uint8_t b = (val >> l) & 0x01;
    Serial.print(b);
  }
  Serial.println();
}

#endif
