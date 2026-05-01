#include "blink_led.h"

Ticker blink(ledFlip, 1000, 0, MILLIS);

enum {
  LED_ON,
  LED_OFF,
  LED_SLOW,
  LED_MEDIUM,
  LED_FAST,
} ledState;

void ledInit() {
  pinMode(LED_BUILTIN, OUTPUT);
  blink.pause();
}

void ledOn() {
  if (ledState != LED_ON) {
    ledState = LED_ON;
    blink.pause();
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void ledOff() {
  if (ledState != LED_OFF) {
    ledState = LED_OFF;
    blink.pause();
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void ledSlow() {
  if (ledState != LED_SLOW) {
    ledState = LED_SLOW;
    blink.interval(1000);
    blink.resume();
  }
}

void ledMedium() {
  if (ledState != LED_MEDIUM) {
    ledState = LED_MEDIUM;
    blink.interval(250);
    blink.resume();
  }
}

void ledFast() {
  if (ledState != LED_FAST) {
    ledState = LED_FAST;
    blink.interval(100);
    blink.resume();
  }
}

void ledFlip() {
  // Invert the current state of the LED_BUILTIN
  digitalWrite(LED_BUILTIN, !(digitalRead(LED_BUILTIN)));  
}
