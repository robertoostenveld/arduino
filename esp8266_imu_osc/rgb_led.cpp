#include "rgb_led.h"

void ledInit() {
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
}

#ifdef COMMON_ANODE
  #define LED_ON  LOW
  #define LED_OFF HIGH
#else
  #define LED_ON  HIGH
  #define LED_OFF LOW
#endif

void ledRed() {
  digitalWrite(LED_R, LED_ON);
  digitalWrite(LED_G, LED_OFF);
  digitalWrite(LED_B, LED_OFF);
}

void ledGreen() {
  digitalWrite(LED_R, LED_OFF);
  digitalWrite(LED_G, LED_ON);
  digitalWrite(LED_B, LED_OFF);
}

void ledBlue() {
  digitalWrite(LED_R, LED_OFF);
  digitalWrite(LED_G, LED_OFF);
  digitalWrite(LED_B, LED_ON);
}

void ledYellow() {
  digitalWrite(LED_R, LED_ON);
  digitalWrite(LED_G, LED_ON);
  digitalWrite(LED_B, LED_OFF);
}

void ledMagenta() {
  digitalWrite(LED_R, LED_ON);
  digitalWrite(LED_G, LED_OFF);
  digitalWrite(LED_B, LED_ON);
}

void ledCyan() {
  digitalWrite(LED_R, LED_OFF);
  digitalWrite(LED_G, LED_ON);
  digitalWrite(LED_B, LED_ON);
}

void ledBlack() {
  digitalWrite(LED_R, LED_OFF);
  digitalWrite(LED_G, LED_OFF);
  digitalWrite(LED_B, LED_OFF);
}

void ledWhite() {
  digitalWrite(LED_R, LED_ON);
  digitalWrite(LED_G, LED_ON);
  digitalWrite(LED_B, LED_ON);
}
