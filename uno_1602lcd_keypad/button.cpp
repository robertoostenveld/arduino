#include "button.h"

// --------------------------------------------------
// Keypad analog input
// --------------------------------------------------

#define BUTTON_PIN A0
#define BUTTON_INTERVAL 20

static int analogValue = 1023;

class KeypadButton : public Bounce2::Button {

public:

  KeypadButton(int lower, int upper) : lower_(lower), upper_(upper) {}

  void init() {
    attach(BUTTON_PIN, INPUT);
    interval(BUTTON_INTERVAL);
  }

protected:

  bool readCurrentState() override {
    return analogValue >= lower_ && analogValue < upper_;
  }

private:

  int lower_;
  int upper_;
};

static KeypadButton buttons[] = {
  KeypadButton(0, 60),
  KeypadButton(60, 200),
  KeypadButton(200, 400),
  KeypadButton(400, 600),
  KeypadButton(600, 800)
};

static const int buttonCount = sizeof(buttons) / sizeof(buttons[0]);

// --------------------------------------------------
// Initialize buttons
// --------------------------------------------------

void initializeButtons() {
  analogValue = analogRead(BUTTON_PIN);
  for (int i = 0; i < buttonCount; i++) {
    buttons[i].init();
  }
}

// --------------------------------------------------
// Update buttons
// --------------------------------------------------

void updateButtons() {
  analogValue = analogRead(BUTTON_PIN);
  for (int i = 0; i < buttonCount; i++) {
    buttons[i].update();
  }
}

// --------------------------------------------------
// Read button
// --------------------------------------------------

Button readButton() {
  for (int i = 0; i < buttonCount; i++) {
    if (buttons[i].pressed()) {
      return static_cast<Button>(i + 1);
    }
  }

  return BTN_NONE;
}
