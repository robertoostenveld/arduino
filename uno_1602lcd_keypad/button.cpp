#include <Arduino.h>
#include "button.h"

// --------------------------------------------------
// Keypad analog input
// --------------------------------------------------

#define BUTTON_PIN A0

// --------------------------------------------------
// Read button
// --------------------------------------------------

Button readButton() {

  int x = analogRead(BUTTON_PIN);

  if (x < 60) {
    return BTN_RIGHT;
  }

  else if (x < 200) {
    return BTN_UP;
  }

  else if (x < 400) {
    return BTN_DOWN;
  }

  else if (x < 600) {
    return BTN_LEFT;
  }

  else if (x < 800) {
    return BTN_SELECT;
  }

  return BTN_NONE;
}