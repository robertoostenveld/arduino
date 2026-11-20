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

enum Mode {
  MODE_SEQUENTIAL,
  MODE_SIMULTANEOUS
};

struct Settings {
  Mode mode;
  int offsetIndex;
  int stepsizeIndex;
  int durationIndex;
  int channelIndex;
};

extern Settings settings;

// --------------------------------------------------
// Menu structure
// --------------------------------------------------

enum MenuType {
  MENU_SUBMENU,
  MENU_ACTION,
  MENU_VALUE_INT,
  MENU_VALUE_FLOAT,
  MENU_OPTION
};

struct MenuItem {
  const char* label;
  int parent;
  int firstChild;
  int nextSibling;
  MenuType type;
  Mode option;
  const int* intValues;
  int intCount;
  int* intValueIndex;
  const float* floatValues;
  int floatCount;
  int* floatValueIndex;
  const char** floatLabels;
};

// --------------------------------------------------
// Menu indices
// --------------------------------------------------

enum MenuIndex {
  MAIN_MENU = 0,

  START,
  SETTINGS,
  INFORMATION,

  MODE_MENU,
  OFFSET,
  STEPSIZE,
  DURATION,
  CHANNELS,
  SAVE_SETTINGS,
  LOAD_SETTINGS,

  SEQUENTIAL_OPTION,
  SIMULTANEOUS_OPTION,

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
void initializeSettings();
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
void saveSettings();
bool loadSettings();

#endif
