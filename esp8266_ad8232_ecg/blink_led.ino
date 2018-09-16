#include "blink_led.h"

Ticker blinker;

void ledInit() {
  pinMode(LED, OUTPUT);
}

void ledOn() {
  digitalWrite(LED, LOW);
}

void ledOff() {
  digitalWrite(LED, HIGH);
}

void changeState() {
  digitalWrite(LED, !(digitalRead(LED)));  // Invert Current State of LED
}

void ledSlow() {
  blinker.detach();
  blinker.attach_ms(1000, changeState);
}

void ledMedium() {
  blinker.detach();
  blinker.attach_ms(250, changeState);
}

void ledFast() {
  blinker.detach();
  blinker.attach_ms(100, changeState);
}


