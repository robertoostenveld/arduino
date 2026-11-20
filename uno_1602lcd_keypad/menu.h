#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <LiquidCrystal.h>
#include "button.h"

// --------------------------------------------------
// LCD object defined in main.ino
// --------------------------------------------------

extern LiquidCrystal lcd;

// --------------------------------------------------
// Application variables
// --------------------------------------------------

extern int brightness;
extern int volume;
extern int operatingMode;
extern int counter;

// --------------------------------------------------
// Menu structure
// --------------------------------------------------

enum MenuType {
  MENU_SUBMENU,
  MENU_ACTION,
  MENU_VALUE,
  MENU_OPTION
};

struct MenuItem {
  const char* label;

  int parent;
  int firstChild;
  int nextSibling;

  MenuType type;

  int* value;
  int minValue;
  int maxValue;
};

// --------------------------------------------------
// Menu indices
// --------------------------------------------------

enum MenuIndex {
  MAIN_MENU = 0,

  SETTINGS,
  MODE_MENU,
  INFORMATION,

  BRIGHTNESS,
  VOLUME,

  MODE_MANUAL,
  MODE_AUTOMATIC,
  MODE_CALIBRATION,

  VERSION,
  COUNTER_MENU,

  MENU_COUNT
};

// --------------------------------------------------
// Menu state
// --------------------------------------------------

extern int currentMenu;
extern int selectedItem;
extern bool editing;
extern int scrollOffset;

// --------------------------------------------------
// Function declarations
// --------------------------------------------------

void initializeMenu();
void displayMenu();
void enterMenu(int item);
void goBack();
void handleButton(Button button);
int getChildCount(int parent);
int getChildAt(int parent, int position);
int getSelectedPosition();
void selectPosition(int position);
void updateScroll();
void executeAction(int item);

#endif