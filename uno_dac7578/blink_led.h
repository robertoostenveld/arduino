#ifndef _BLINK_LED_H_
#define _BLINK_LED_H_

#include <Arduino.h>
#include <Ticker.h>  // see https://github.com/sstaub/Ticker

extern Ticker blinker;

void ledInit(void);
void ledOn(void);
void ledOff(void);
void ledSlow(void);
void ledMedium(void);
void ledFast(void);
void ledFlip(void);

#endif