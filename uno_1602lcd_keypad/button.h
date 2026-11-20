#ifndef BUTTON_H
#define BUTTON_H

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

Button readButton();

#endif