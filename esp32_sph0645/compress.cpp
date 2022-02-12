#include <Arduino.h>

uint32_t compress(uint32_t *data, uint32_t nsamples, uint8_t *dest) {
  uint32_t nbytes = 0;
  uint32_t *ptr = (uint32_t *)dest;
  for (int i; i < nsamples; i++) {
    ptr[i] = data[i];
    nbytes += 4;
  }
  return nbytes;
} // compress


uint32_t decompress(uint32_t *dest, uint32_t nsamples, uint8_t *data) {
  uint32_t nbytes = 0;
  uint32_t *ptr = (uint32_t *)data;
  for (int i; i < nsamples; i++) {
    dest[i] = ptr[i];
    nbytes += 4;
  }
  return nbytes;
} // decompress
