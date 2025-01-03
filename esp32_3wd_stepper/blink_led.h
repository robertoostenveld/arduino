#ifndef _BLINK_LED_H_
#define _BLINK_LED_H_

#include <Arduino.h>
#include <Ticker.h>

#define LED 22 // GPIO22 is connected to the builtin LED on the Lolin32 Lite

void ledInit(void);
void ledOn(void);
void ledOff(void);
void ledSlow(void);
void ledMedium(void);
void ledFast(void);

#endif
