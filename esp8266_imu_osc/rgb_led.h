#ifndef _RGB_LED_H_
#define _RGB_LED_H_

#include <Arduino.h>

#define LED_R D5
#define LED_G D6
#define LED_B D7

// #define COMMON_ANODE

void ledInit();

void ledRed();
void ledGreen();
void ledBlue();

void ledYellow();
void ledMagenta();
void ledCyan();

void ledBlack();

#endif
