#ifndef _RGB_LED_H_
#define _RGB_LED_H_

#define LED_R D1
#define LED_G D2
#define LED_B D3

// #define COMMON_ANODE

void ledInit();

void ledRed();
void ledGreen();
void ledBlue();

void ledYellow();
void ledMagenta();
void ledCyan();

void ledBlack();
void ledWhite();

#endif

