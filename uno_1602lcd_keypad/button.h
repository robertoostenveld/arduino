#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include <Bounce2.h>

// --------------------------------------------------
// Button definitions
// --------------------------------------------------

enum Button {
  BTN_NONE,
  BTN_RIGHT,
  BTN_UP,
  BTN_DOWN,
  BTN_LEFT,
  BTN_SELECT
};

// --------------------------------------------------
// Function declarations
// --------------------------------------------------

void initializeButtons();
void updateButtons();
Button readButton();

#endif