#include <Arduino.h>
#include "neopixel_mode.h"

// the neopixel strip is connected to pin 0
#define BUILTIN_LED 1
#define BUTTON0     2
#define BUTTON1     3
#define BUTTON2     4

// mode1  details are specified as r, g, b
// mode4  details are specified as r1, g1, b1, r2, g2, b2, speed
// mode10 details are specified as r1, g1, b1, r2, g2, b2, speed
// mode12 details are specified as saturation, value, speed

uint8_t specification0[] = {255, 255, 255};
uint8_t specification1[] = {255, 0, 0, 0, 255, 0, 2};
uint8_t specification2[] = {0, 255, 0, 0, 0, 255, 2};
uint8_t specification3[] = {0, 0, 255, 255, 0, 0, 2};
uint8_t specification4[] = {255, 0, 0, 0, 255, 0, 2};
uint8_t specification5[] = {0, 255, 0, 0, 0, 255, 2};
uint8_t specification6[] = {0, 0, 255, 255, 0, 0, 2};
uint8_t specification7[] = {220, 255, 1};

unsigned long previous = 0, now;

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(BUTTON0, INPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  strip.begin(); // this is declared in neopixel_mode
} // setup

void loop() {
  // blink the builtin LED for diagnostics
  now = millis();
  if ((now - previous) > 1000) {
    digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
    previous = now;
  }

  // combine the three buttons in a single binary mode
  uint8_t mode = (digitalRead(BUTTON0) << 0) | (digitalRead(BUTTON1) << 1) | (digitalRead(BUTTON2) << 2);

  // update the Neopixel LED strip according to the current mode
  switch (mode) {
    case 0:
      mode1(specification0);
      break;
    case 1:
      mode4(specification1);
      break;
    case 2:
      mode4(specification2);
      break;
    case 3:
      mode4(specification3);
      break;
    case 4:
      mode10(specification4);
      break;
    case 5:
      mode10(specification5);
      break;
    case 6:
      mode10(specification6);
      break;
    case 7:
      mode12(specification7);
      break;
  }
} // loop
