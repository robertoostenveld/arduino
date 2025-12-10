#include "blink_led.h"

Ticker blinker;

enum {
  LED_ON,
  LED_OFF,
  LED_SLOW,
  LED_MEDIUM,
  LED_FAST,
} ledState;

void changeState() {
  digitalWrite(LED, !(digitalRead(LED)));  // Invert the current state of the LED
}

void ledInit() {
  pinMode(LED, OUTPUT);
}

void ledOn() {
  if (ledState != LED_ON) {
    ledState = LED_ON;
    blinker.detach();
    digitalWrite(LED, LOW);
  }
}

void ledOff() {
  if (ledState != LED_OFF) {
    ledState = LED_OFF;
    blinker.detach();
    digitalWrite(LED, HIGH);
  }
}

void ledSlow() {
  if (ledState != LED_SLOW) {
    ledState = LED_SLOW;
    blinker.detach();
    blinker.attach_ms(1000, changeState);
  }
}

void ledMedium() {
  if (ledState != LED_MEDIUM) {
    ledState = LED_MEDIUM;
    blinker.detach();
    blinker.attach_ms(250, changeState);
  }
}

void ledFast() {
  if (ledState != LED_FAST) {
    ledState = LED_FAST;
    blinker.detach();
    blinker.attach_ms(100, changeState);
  }
}