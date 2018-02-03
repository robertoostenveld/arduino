#ifndef _RGB_LED_H_
#define _RGB_LED_H_

#define LED_R D6
#define LED_G LED_BUILTIN
#define LED_B D8

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

