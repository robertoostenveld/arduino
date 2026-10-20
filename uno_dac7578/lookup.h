#ifndef _LOOKUP_H_
#define _LOOKUP_H_

#include <Arduino.h>

#ifndef M_2PI
#define M_2PI 6.28318530717958647692528676655900576
#endif

#define LOOKUP_LEN 256                // see the tables in lookup.cpp for the supported values
#define LOOKUP_MASK (LOOKUP_LEN - 1)  // Using this as binary mask is faster than taking the modulo 
#define PHASE_TO_INDEX ((float)LOOKUP_LEN / M_2PI)

extern uint16_t lookup_value[];

uint16_t lookup_floor(float);
uint16_t lookup_nearest(float);
uint16_t lookup_interpolate(float);

#endif