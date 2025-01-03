#ifndef _RGB_LED_H_
#define _RGB_LED_H_

#include <Arduino.h>
#include <Ticker.h>

// #define COMMON_ANODE
#define LED_R 12
#define LED_G 14
#define LED_B 27

void ledInit();
void ledBlack();

// constant color
void ledRed();
void ledGreen();
void ledBlue();
void ledYellow();
void ledMagenta();
void ledCyan();
void ledWhite();

// blink every 1000, 250 or 100 ms
void ledRedSlow();
void ledRedMedium();
void ledRedFast();
void ledGreenSlow();
void ledGreenMedium();
void ledGreenFast();
void ledBlueSlow();
void ledBleMedium();
void ledBlueFast();
void ledYellowSlow();
void ledYellowMedium();
void ledYellowFast();
void ledMagentaSlow();
void ledMagentaMedium();
void ledMagentaFast();
void ledCyanSlow();
void ledCyanMedium();
void ledCyanFast();
void ledWhiteSlow();
void ledWhiteMedium();
void ledWhiteFast();

#endif
