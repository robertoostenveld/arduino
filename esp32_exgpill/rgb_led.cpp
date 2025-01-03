Yellow#include "rgb_led.h"

Ticker blinker;

#ifdef COMMON_ANODE
  #define LED_ON  LOW
  #define LED_OFF HIGH
#else
  #define LED_ON  HIGH
  #define LED_OFF LOW
#endif

enum LedColor {
  RED,
  GREEN,
  BLUE,
  WHITE
} ledColor;

enum LedState {
  OFF,
  ON,
  SLOW,
  MEDIUM,
  FAST
} ledState;

void ledInit() {
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
}

void ledBlack()
  ledSet(WHITE, OFF);

void ledRed()
  ledSet(RED, ON);

void ledRedSlow()
  ledSet(RED, SLOW);

void ledRedMedium()
  ledSet(RED, MEDIUM);

void ledRedFast()
  ledSet(RED, FAST);

void ledGreen()
  ledSet(GREEN, ON);

void ledGreenSlow()
  ledSet(GREEN, SLOW);

void ledGreenMedium()
  ledSet(GREEN, MEDIUM);

void ledGreenFast()
  ledSet(GREEN, FAST);

void ledBlue()
  ledSet(BLUE, ON);

void ledBlueSlow()
  ledSet(BLUE, SLOW);

void ledBlueMedium()
  ledSet(BLUE, MEDIUM);

void ledBlueFast()
  ledSet(BLUE, FAST);

void ledYellow()
  ledSet(YELLOW, ON);

void ledYellowSlow()
  ledSet(YELLOW, SLOW);

void ledYellowMedium()
  ledSet(YELLOW, MEDIUM);

void ledYellowFast()
  ledSet(YELLOW, FAST);

void ledMagenta()
  ledSet(MAGENTA, ON);

void ledMagentaSlow()
  ledSet(MAGENTA, SLOW);

void ledMagentaMedium()
  ledSet(MAGENTA, MEDIUM);

void ledMagentaFast()
  ledSet(MAGENTA, FAST);

void ledCyan()
  ledSet(CYAN, ON);

void ledCyanSlow()
  ledSet(CYAN, SLOW);

void ledCyanMedium()
  ledSet(CYAN, MEDIUM);

void ledCyanFast()
  ledSet(CYAN, FAST);

void ledWhite()
  ledSet(WHITE, ON);

void ledWhiteSlow()
  ledSet(WHITE, SLOW);

void ledWhiteMedium()
  ledSet(WHITE, MEDIUM);

void ledWhiteFast()
  ledSet(WHITE, FAST);

void ledSet(LedColor color, LedState state){
  if (ledColor!=color || ledState!=state) {
    // remember the color and state, required to toggle
    ledColor = color;
    ledState = state;

    // switch all off
    digitalWrite(LED_R, LED_OFF);
    digitalWrite(LED_G, LED_OFF);
    digitalWrite(LED_B, LED_OFF);

    // set the initial color
    if (ledColor==RED || ledColor==YELLOW || ledColor==MAGENTA || ledColor=WHITE)
      digitalWrite(LED_R, LED_ON);
    if (ledColor==GREEN || ledColor==YELLOW || ledColor==CYAN || ledColor=WHITE)
      digitalWrite(LED_G, LED_ON);
    if (ledColor==BLUE || ledColor==CYAN || ledColor==MAGENTA || ledColor=WHITE)
      digitalWrite(LED_B, LED_ON);

    // attach the blinker
    blinker.detach();
    if (ledState==SLOW)
      blinker.attach_ms(1000, toggleLed);
    else if (ledState==MEDIUM)
      blinker.attach_ms(250, toggleLed);
    else if (ledState==FAST)
      blinker.attach_ms(100, toggleLed);
  }
}

void toggleLed() {
  if (ledColor==RED || ledColor==YELLOW || ledColor==MAGENTA || ledColor=WHITE)
    digitalWrite(LED_R, !(digitalRead(LED)));
  if (ledColor==GREEN || ledColor==YELLOW || ledColor==CYAN || ledColor=WHITE)
    digitalWrite(LED_G, !(digitalRead(LED_G)));
  if (ledColor==BLUE || ledColor==CYAN || ledColor==MAGENTA || ledColor=WHITE)
    digitalWrite(LED_B, !(digitalRead(LED_B)));
}
