#include <Arduino.h>
#include "menu.h"

// --------------------------------------------------
// Application variables
// --------------------------------------------------

int brightness = 5;
int volume = 10;
int operatingMode = 0;
int counter = 0;

// --------------------------------------------------
// Menu state
// --------------------------------------------------

int currentMenu = MAIN_MENU;
int selectedItem = SETTINGS;

bool editing = false;

int scrollOffset = 0;

// --------------------------------------------------
// Menu definition
// --------------------------------------------------

MenuItem menu[MENU_COUNT] = {

  // label, parent, firstChild, nextSibling, type,
  // value, minValue, maxValue

  { "Main menu", -1, SETTINGS, -1,
    MENU_SUBMENU, nullptr, 0, 0 },

  { "Settings", MAIN_MENU, BRIGHTNESS, MODE_MENU,
    MENU_SUBMENU, nullptr, 0, 0 },

  { "Mode", MAIN_MENU, MODE_MANUAL, INFORMATION,
    MENU_SUBMENU, nullptr, 0, 0 },

  { "Information", MAIN_MENU, VERSION, -1,
    MENU_SUBMENU, nullptr, 0, 0 },

  { "Brightness", SETTINGS, -1, VOLUME,
    MENU_VALUE, &brightness, 0, 10 },

  { "Volume", SETTINGS, -1, -1,
    MENU_VALUE, &volume, 0, 20 },

  { "Manual", MODE_MENU, -1, MODE_AUTOMATIC,
    MENU_OPTION, &operatingMode, 0, 0 },

  { "Automatic", MODE_MENU, -1, MODE_CALIBRATION,
    MENU_OPTION, &operatingMode, 0, 1 },

  { "Calibration", MODE_MENU, -1, -1,
    MENU_OPTION, &operatingMode, 0, 2 },

  { "Version", INFORMATION, -1, COUNTER_MENU,
    MENU_ACTION, nullptr, 0, 0 },

  { "Counter", INFORMATION, -1, -1,
    MENU_ACTION, nullptr, 0, 0 }
};

// --------------------------------------------------
// Initialize menu
// --------------------------------------------------

void initializeMenu() {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Nested menu");

  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  delay(500);

  enterMenu(MAIN_MENU);

  selectedItem = SETTINGS;

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

    if (menu[selectedItem].value != nullptr) {
      lcd.print(*menu[selectedItem].value);
    }

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
  }

  // Second row

  lcd.setCursor(0, 1);

  if (child2 != -1) {
    lcd.print(child2 == selectedItem ? ">" : " ");
    lcd.print(menu[child2].label);
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
                     ? SETTINGS
                     : menu[currentMenu].firstChild;

    scrollOffset = 0;
  }
}

// --------------------------------------------------
// Actions
// --------------------------------------------------

void executeAction(int item) {

  switch (item) {

    case VERSION:

      lcd.clear();
      lcd.print("Version 1.0");

      delay(1000);

      break;

    case COUNTER_MENU:

      counter++;

      lcd.clear();
      lcd.print("Counter:");
      lcd.print(counter);

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

    selectedItem = SETTINGS;
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

      if (*item->value < item->maxValue)
        (*item->value)++;
    }

    if (button == BTN_DOWN) {

      if (*item->value > item->minValue)
        (*item->value)--;
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

        operatingMode = item->minValue;

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