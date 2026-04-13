#include "blink_led.h"

Ticker blinker(ledFlip, 1000, 0, MILLIS);

enum {
  LED_ON,
  LED_OFF,
  LED_SLOW,
  LED_MEDIUM,
  LED_FAST,
} ledState;

void ledInit() {
  pinMode(LED_BUILTIN, OUTPUT);
  blinker.pause();
}

void ledOn() {
  if (ledState != LED_ON) {
    ledState = LED_ON;
    blinker.pause();
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void ledOff() {
  if (ledState != LED_OFF) {
    ledState = LED_OFF;
    blinker.pause();
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void ledSlow() {
  if (ledState != LED_SLOW) {
    ledState = LED_SLOW;
    blinker.interval(1000);
    blinker.resume();
  }
}

void ledMedium() {
  if (ledState != LED_MEDIUM) {
    ledState = LED_MEDIUM;
    blinker.interval(250);
    blinker.resume();
  }
}

void ledFast() {
  if (ledState != LED_FAST) {
    ledState = LED_FAST;
    blinker.interval(100);
    blinker.resume();
  }
}

void ledFlip() {
  // Invert the current state of the LED_BUILTIN
  digitalWrite(LED_BUILTIN, !(digitalRead(LED_BUILTIN)));  
}
