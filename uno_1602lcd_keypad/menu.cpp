#include <Arduino.h>
#include <EEPROM.h>
#include "menu.h"

// --------------------------------------------------
// Application variables
// --------------------------------------------------

Settings settings = {
  MODE_SINGLE,
  0,
  0,
  0
};

// --------------------------------------------------
// Pre-defined values
// --------------------------------------------------

static const float offsetValues[] = { 0.0, 0.1, 0.5, 1.0, 2.0, 5.0 };
static const float stepsizeValues[] = { 0.01, 0.05, 0.1, 0.5, 1.0 };
static const float durationValues[] = { 1.0, 2.0, 5.0, 10.0, 30.0 };

static const int offsetCount = sizeof(offsetValues) / sizeof(offsetValues[0]);
static const int stepsizeCount = sizeof(stepsizeValues) / sizeof(stepsizeValues[0]);
static const int durationCount = sizeof(durationValues) / sizeof(durationValues[0]);

static const char* modeNames[] = { "Sgl", "Seq", "Sim" };

// --------------------------------------------------
// EEPROM storage
// --------------------------------------------------

#define EEPROM_ADDRESS 0
#define EEPROM_MAGIC 0xA57C

struct StoredSettings {
  unsigned int magic;
  Mode mode;
  int offsetIndex;
  int stepsizeIndex;
  int durationIndex;
};

// --------------------------------------------------
// Menu state
// --------------------------------------------------

int currentMenu = MAIN_MENU;
int selectedItem = START;

bool editing = false;

int scrollOffset = 0;

// --------------------------------------------------
// Menu item constructors
// --------------------------------------------------

MenuItem::MenuItem(const char* label, int parent, int firstChild, int nextSibling, MenuType type)
  : label(label), parent(parent), firstChild(firstChild), nextSibling(nextSibling), type(type),
    modeValue(nullptr), option(MODE_SINGLE), values(nullptr), valueCount(0), valueIndex(nullptr) {}

MenuItem::MenuItem(const char* label, int parent, int firstChild, int nextSibling, MenuType type,
                   Mode* modeValue, Mode option)
  : label(label), parent(parent), firstChild(firstChild), nextSibling(nextSibling), type(type),
    modeValue(modeValue), option(option), values(nullptr), valueCount(0), valueIndex(nullptr) {}

MenuItem::MenuItem(const char* label, int parent, int firstChild, int nextSibling, MenuType type,
                   const float* values, int valueCount, int* valueIndex)
  : label(label), parent(parent), firstChild(firstChild), nextSibling(nextSibling), type(type),
    modeValue(nullptr), option(MODE_SINGLE), values(values), valueCount(valueCount), valueIndex(valueIndex) {}

// --------------------------------------------------
// Menu definition
// --------------------------------------------------

MenuItem menu[MENU_COUNT] = {

  // label, parent, firstChild, nextSibling, type,
  // value information (depending on type)

  { "Main menu", -1, START, -1,
    MENU_SUBMENU },

  { "Start", MAIN_MENU, -1, SETTINGS,
    MENU_ACTION },

  { "Settings", MAIN_MENU, MODE_MENU, INFORMATION,
    MENU_SUBMENU },

  { "Information", MAIN_MENU, INFO_SETTINGS, -1,
    MENU_SUBMENU },

  { "Mode", SETTINGS, SINGLE_OPTION, OFFSET,
    MENU_SUBMENU },

  { "Offset", SETTINGS, -1, STEPSIZE,
    MENU_VALUE, offsetValues, offsetCount, &settings.offsetIndex },

  { "Stepsize", SETTINGS, -1, DURATION,
    MENU_VALUE, stepsizeValues, stepsizeCount, &settings.stepsizeIndex },

  { "Duration", SETTINGS, -1, SAVE_SETTINGS,
    MENU_VALUE, durationValues, durationCount, &settings.durationIndex },

  { "Save settings", SETTINGS, -1, LOAD_SETTINGS, MENU_ACTION },

  { "Load settings", SETTINGS, -1, -1,
    MENU_ACTION },

  { "Single", MODE_MENU, -1, SEQUENTIAL_OPTION,
    MENU_OPTION, &settings.mode, MODE_SINGLE },

  { "Sequential", MODE_MENU, -1, SIMULTANEOUS_OPTION,
    MENU_OPTION, &settings.mode, MODE_SEQUENTIAL },

  { "Simultaneous", MODE_MENU, -1, -1,
    MENU_OPTION, &settings.mode, MODE_SIMULTANEOUS },

  { "Settings", INFORMATION, -1, VERSION,
    MENU_ACTION },

  { "Version", INFORMATION, -1, COPYRIGHT,
    MENU_ACTION },

  { "Copyright", INFORMATION, -1, -1,
    MENU_ACTION }
};

// --------------------------------------------------
// Initialize menu
// --------------------------------------------------

void initializeMenu() {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Mode control");

  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  delay(500);

  enterMenu(MAIN_MENU);

  selectedItem = START;

  displayMenu();
}

// --------------------------------------------------
// Helper functions
// --------------------------------------------------

int getChildCount(int parent) {

  int count = 0;
  int child = menu[parent].firstChild;

  while (child != -1) {
    count++;
    child = menu[child].nextSibling;
  }

  return count;
}

// --------------------------------------------------

int getChildAt(int parent, int position) {

  int child = menu[parent].firstChild;

  for (int i = 0; i < position && child != -1; i++) {
    child = menu[child].nextSibling;
  }

  return child;
}

// --------------------------------------------------

int getSelectedPosition() {

  int child = menu[currentMenu].firstChild;
  int position = 0;

  while (child != -1) {

    if (child == selectedItem)
      return position;

    child = menu[child].nextSibling;
    position++;
  }

  return 0;
}

// --------------------------------------------------

void selectPosition(int position) {

  selectedItem = getChildAt(currentMenu, position);
}

// --------------------------------------------------

void updateScroll() {

  int position = getSelectedPosition();

  if (position < scrollOffset)
    scrollOffset = position;

  if (position >= scrollOffset + 2)
    scrollOffset = position - 1;
}

// --------------------------------------------------

static void printValue(float value, int column, int row) {

  char buffer[8];

  dtostrf(value, 5, 2, buffer);

  lcd.setCursor(column, row);
  lcd.print(buffer);
}

// --------------------------------------------------

static void showValue(int item, int row) {

  MenuItem* current = &menu[item];

  if (current->type == MENU_VALUE) {
    printValue(current->values[*current->valueIndex], 11, row);
  }

  if (current->type == MENU_OPTION && *current->modeValue == current->option) {
    lcd.setCursor(15, row);
    lcd.print('*');
  }
}

// --------------------------------------------------
// Display
// --------------------------------------------------

void displayMenu() {

  lcd.clear();

  // ------------------------------------------------
  // Editing a value
  // ------------------------------------------------

  if (editing) {

    lcd.setCursor(0, 0);
    lcd.print("Edit:");
    lcd.print(menu[selectedItem].label);

    lcd.setCursor(0, 1);
    lcd.print("< ");
    lcd.print(menu[selectedItem].values[*menu[selectedItem].valueIndex], 2);
    lcd.print(" >");

    return;
  }

  // ------------------------------------------------
  // Normal menu display
  // ------------------------------------------------

  int child1 = getChildAt(currentMenu, scrollOffset);
  int child2 = getChildAt(currentMenu, scrollOffset + 1);

  // First row

  lcd.setCursor(0, 0);

  if (child1 != -1) {
    lcd.print(child1 == selectedItem ? ">" : " ");
    lcd.print(menu[child1].label);
    showValue(child1, 0);
  }

  // Second row

  lcd.setCursor(0, 1);

  if (child2 != -1) {
    lcd.print(child2 == selectedItem ? ">" : " ");
    lcd.print(menu[child2].label);
    showValue(child2, 1);
  }
}

// --------------------------------------------------
// Navigation
// --------------------------------------------------

void enterMenu(int item) {

  currentMenu = item;

  selectedItem = menu[item].firstChild;

  scrollOffset = 0;

  editing = false;
}

// --------------------------------------------------

void goBack() {

  if (editing) {
    editing = false;
    return;
  }

  if (currentMenu != MAIN_MENU) {

    int parent = menu[currentMenu].parent;

    currentMenu = parent;

    selectedItem = currentMenu == MAIN_MENU
                     ? START
                     : menu[currentMenu].firstChild;

    scrollOffset = 0;
  }
}

// --------------------------------------------------
// Settings storage
// --------------------------------------------------

void saveSettings() {

  StoredSettings stored = {
    EEPROM_MAGIC,
    settings.mode,
    settings.offsetIndex,
    settings.stepsizeIndex,
    settings.durationIndex
  };

  EEPROM.put(EEPROM_ADDRESS, stored);
}

// --------------------------------------------------

bool loadSettings() {

  StoredSettings stored;

  EEPROM.get(EEPROM_ADDRESS, stored);

  if (stored.magic != EEPROM_MAGIC)
    return false;

  if (stored.mode <= MODE_SIMULTANEOUS)
    settings.mode = stored.mode;

  settings.offsetIndex = constrain(stored.offsetIndex, 0, offsetCount - 1);
  settings.stepsizeIndex = constrain(stored.stepsizeIndex, 0, stepsizeCount - 1);
  settings.durationIndex = constrain(stored.durationIndex, 0, durationCount - 1);

  return true;
}

// --------------------------------------------------
// Actions
// --------------------------------------------------

void executeAction(int item) {

  switch (item) {

    case START:

      lcd.clear();
      lcd.print("Started");

      lcd.setCursor(0, 1);
      lcd.print(modeNames[settings.mode]);

      delay(1000);

      break;

    case SAVE_SETTINGS:

      saveSettings();

      lcd.clear();
      lcd.print("Settings saved");

      delay(1000);

      break;

    case LOAD_SETTINGS:
      {

        bool loaded = loadSettings();

        lcd.clear();
        lcd.print(loaded ? "Settings loaded" : "No saved data");

        delay(1000);

        break;
      }

    case INFO_SETTINGS:

      lcd.clear();
      lcd.print("O:");
      lcd.print(offsetValues[settings.offsetIndex], 2);
      lcd.print(" S:");
      lcd.print(stepsizeValues[settings.stepsizeIndex], 2);

      lcd.setCursor(0, 1);
      lcd.print("D:");
      lcd.print(durationValues[settings.durationIndex], 2);
      lcd.print(" ");
      lcd.print(modeNames[settings.mode]);

      delay(1000);

      break;

    case VERSION:

      lcd.clear();
      lcd.print("Version 1.0");

      delay(1000);

      break;

    case COPYRIGHT:

      lcd.clear();
      lcd.print("Copyright (c) 2026");

      lcd.setCursor(0, 1);
      lcd.print("All rights reserved");

      delay(1000);

      break;
  }
}

// --------------------------------------------------
// Button handling
// --------------------------------------------------

void handleButton(Button button) {

  if (button == BTN_NONE)
    return;

  // ------------------------------------------------
  // SELECT: return to top-level menu
  // ------------------------------------------------

  if (button == BTN_SELECT) {

    editing = false;

    enterMenu(MAIN_MENU);

    selectedItem = START;
    scrollOffset = 0;

    displayMenu();

    return;
  }

  // ------------------------------------------------
  // Editing a value
  // ------------------------------------------------

  if (editing) {

    MenuItem* item = &menu[selectedItem];

    if (button == BTN_UP) {

      if (*item->valueIndex < item->valueCount - 1)
        (*item->valueIndex)++;
    }

    if (button == BTN_DOWN) {

      if (*item->valueIndex > 0)
        (*item->valueIndex)--;
    }

    if (button == BTN_LEFT || button == BTN_RIGHT) {

      editing = false;
    }

    displayMenu();

    return;
  }

  // ------------------------------------------------
  // Normal menu navigation
  // ------------------------------------------------

  if (button == BTN_UP) {

    int position = getSelectedPosition();

    if (position > 0)
      selectPosition(position - 1);
  }

  else if (button == BTN_DOWN) {

    int position = getSelectedPosition();
    int count = getChildCount(currentMenu);

    if (position < count - 1)
      selectPosition(position + 1);
  }

  else if (button == BTN_LEFT) {

    goBack();
  }

  else if (button == BTN_RIGHT) {

    MenuItem* item = &menu[selectedItem];

    switch (item->type) {

      case MENU_SUBMENU:

        enterMenu(selectedItem);

        break;

      case MENU_VALUE:

        editing = true;

        break;

      case MENU_OPTION:

        *item->modeValue = item->option;

        lcd.clear();
        lcd.print("Mode selected:");

        lcd.setCursor(0, 1);
        lcd.print(item->label);

        delay(500);

        break;

      case MENU_ACTION:

        executeAction(selectedItem);

        break;
    }
  }

  // ------------------------------------------------
  // Update scrolling and display
  // ------------------------------------------------

  updateScroll();

  displayMenu();
}
