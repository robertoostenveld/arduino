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
  MODE_SINGLE,
  MODE_SEQUENTIAL,
  MODE_SIMULTANEOUS
};

struct Settings {
  Mode mode;
  int offsetIndex;
  int stepsizeIndex;
  int durationIndex;
};

extern Settings settings;

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

  Mode* modeValue;
  Mode option;

  const float* values;
  int valueCount;
  int* valueIndex;

  MenuItem(const char* label, int parent, int firstChild, int nextSibling, MenuType type);
  MenuItem(const char* label, int parent, int firstChild, int nextSibling, MenuType type,
           Mode* modeValue, Mode option);
  MenuItem(const char* label, int parent, int firstChild, int nextSibling, MenuType type,
           const float* values, int valueCount, int* valueIndex);
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
  SAVE_SETTINGS,
  LOAD_SETTINGS,

  SINGLE_OPTION,
  SEQUENTIAL_OPTION,
  SIMULTANEOUS_OPTION,

  INFO_SETTINGS,
  VERSION,
  COPYRIGHT,

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
void saveSettings();
bool loadSettings();

#endif
