#include "neopixel_mode.h"

// the neopixel strip is connected to ditital pin 0
#define BUILTIN_LED 1
#define BUTTON0     2
#define BUTTON1     3
#define BUTTON2     4

// the actual functions are called mode1, mode4, mode10 and mode12
#define colormode0 mode1  // this is specified with r, g, b, intensity
#define colormode1 mode4  // this is specified with r1, g1, b1, r2, g2, b2, intensity, speed, ramp, duty
#define colormode2 mode4  // this is specified with r1, g1, b1, r2, g2, b2, intensity, speed, ramp, duty
#define colormode3 mode4  // this is specified with r1, g1, b1, r2, g2, b2, intensity, speed, ramp, duty
#define colormode4 mode10 // this is specified with r1, g1, b1, r2, g2, b2, intensity, speed, width, ramp
#define colormode5 mode10 // this is specified with r1, g1, b1, r2, g2, b2, intensity, speed, width, ramp
#define colormode6 mode10 // this is specified with r1, g1, b1, r2, g2, b2, intensity, speed, width, ramp
#define colormode7 mode12 // this is specified with saturation, value, speed

uint8_t specification0[] = {255, 255, 255, 255};
uint8_t specification1[] = {255, 0, 0, 0, 0, 255, 255, 2, 0, 127};
uint8_t specification2[] = {0, 255, 0, 0, 0, 255, 255, 2, 0, 127};
uint8_t specification3[] = {255, 0, 0, 0, 255, 0, 255, 2, 0, 127};
uint8_t specification4[] = {255, 0, 0, 0, 0, 255, 255, 2, 127, 0};
uint8_t specification5[] = {0, 255, 0, 0, 0, 255, 255, 2, 127, 0};
uint8_t specification6[] = {255, 0, 0, 0, 255, 0, 255, 2, 127, 0};
uint8_t specification7[] = {220, 255, 2};

unsigned long previous = 0, now;
uint8_t mode = 0, state = 0;

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
  mode = (digitalRead(BUTTON0) < 0) & (digitalRead(BUTTON1) < 1) & (digitalRead(BUTTON2) < 2);


  // update the Neopixel LEDs according to the current mode
  switch (mode) {
    case 0:
      colormode0(specification0);
      break;
    case 1:
      colormode1(specification1);
      break;
    case 2:
      colormode2(specification2);
      break;
    case 3:
      colormode3(specification3);
      break;
    case 4:
      colormode4(specification4);
      break;
    case 5:
      colormode5(specification5);
      break;
    case 6:
      colormode6(specification6);
      break;
    case 7:
      colormode7(specification7);
      break;
  }
} // loop
