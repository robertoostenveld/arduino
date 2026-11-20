#include <LiquidCrystal.h>
#include "button.h"
#include "menu.h"

// --------------------------------------------------
// LCD pin configuration
// --------------------------------------------------

const int pin_RS = 8;
const int pin_EN = 9;
const int pin_d4 = 4;
const int pin_d5 = 5;
const int pin_d6 = 6;
const int pin_d7 = 7;
const int pin_BL = 10;

LiquidCrystal lcd(pin_RS, pin_EN, pin_d4, pin_d5, pin_d6, pin_d7);

// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {

  lcd.begin(16, 2);

  pinMode(pin_BL, OUTPUT);
  digitalWrite(pin_BL, HIGH);

  initializeMenu();
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {

  Button button = readButton();

  if (button != BTN_NONE) {

    handleButton(button);

    // Wait for button release
    while (readButton() != BTN_NONE) {
      delay(10);
    }

    delay(150);  // Debounce
  }
}