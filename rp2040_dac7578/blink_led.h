#ifndef _BLINK_LED_H_
#define _BLINK_LED_H_

#include <Arduino.h>
#include <Ticker.h>

#define LED D13 //connected to the builtin LED on the Adafruit Feather RP2040

void ledInit(void);
void ledOn(void);
void ledOff(void);
void ledSlow(void);
void ledMedium(void);
void ledFast(void);

#endif