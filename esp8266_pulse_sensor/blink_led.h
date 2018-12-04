#ifndef _BLINK_LED_H_
#define _BLINK_LED_H_
#include <Ticker.h>

#define LED 2 // GPIO2 is connected to the builtin LED

void ledInit(void);
void ledOn(void);
void ledOff(void);
void ledSlow(void);
void ledMedium(void);
void ledFast(void);

#endif
