#include <Arduino.h>
#include <EEPROM.h>
#include <string.h>
#include "menu.h"
#include "version.h"

// --------------------------------------------------
// Application variables
// --------------------------------------------------

static const Settings defaultSettings = {
  MODE_SEQUENTIAL,
  10, // offsetIndex
  6,  // stepsizeIndex
  7,  // durationIndex
  23  // channelIndex
};

Settings settings = defaultSettings;

// --------------------------------------------------
// Pre-defined values
// --------------------------------------------------

static const float offsetValues[] = { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0, 25.0, 26.0, 27.0, 28.0, 29.0, 30.0 };
static const float stepsizeValues[] = { 1./9, 1./8, 1./7, 1./6, 1./5, 1./4, 1./3, 1./2, 1./1 };
static const char* stepsizeLabels[] = { "1/9", "1/8", "1/7", "1/6", "1/5", "1/4", "1/3", "1/2", "1/1" };
static const float durationValues[] = { 1.0, 2.0, 5.0, 10.0, 20.0, 30.0, 60.0, 120.0, 240.0, 300.0, 9999.0 };
static const int channelValues[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 };

static const int offsetCount = sizeof(offsetValues) / sizeof(offsetValues[0]);
static const int stepsizeCount = sizeof(stepsizeValues) / sizeof(stepsizeValues[0]);
static const int durationCount = sizeof(durationValues) / sizeof(durationValues[0]);
static const int channelCount = sizeof(channelValues) / sizeof(channelValues[0]);

static const char* modeNames[] = { "Seq", "Sim" };

// --------------------------------------------------
// EEPROM storage
// --------------------------------------------------

#define EEPROM_ADDRESS 0
#define EEPROM_MAGIC 0xA57D
#define EEPROM_SIGNATURE_SIZE 16

struct StoredSettings {
  unsigned int magic;
  char signature[EEPROM_SIGNATURE_SIZE];
  Mode mode;
  int offsetIndex;
  int stepsizeIndex;
  int durationIndex;
  int channelIndex;
};

// --------------------------------------------------
// Menu state
// --------------------------------------------------

int currentMenu = MAIN_MENU;
int selectedItem = START;

bool editing = false;

int scrollOffset = 0;

static bool showingInfo = false;

// --------------------------------------------------
// Menu definition
// --------------------------------------------------

// clang-format off
MenuItem menu[MENU_COUNT] = {
  // idx            label            parent        firstChild          nextSibling         type                 option              int*,n,pInt           float*,n,pFloat,labels
  /* MAIN_MENU      */ { "Main menu",     -1,           START,              -1,                 MENU_SUBMENU,        MODE_SEQUENTIAL,    NULL,0,NULL,          NULL,0,NULL,NULL },
  /* START          */ { "Start",         MAIN_MENU,    -1,                 SETTINGS,           MENU_ACTION,         MODE_SEQUENTIAL,    NULL,0,NULL,          NULL,0,NULL,NULL },
  /* SETTINGS       */ { "Settings",      MAIN_MENU,    MODE_MENU,          INFORMATION,        MENU_SUBMENU,        MODE_SEQUENTIAL,    NULL,0,NULL,          NULL,0,NULL,NULL },
  /* INFORMATION    */ { "Information",   MAIN_MENU,    -1,                 -1,                 MENU_SUBMENU,        MODE_SEQUENTIAL,    NULL,0,NULL,          NULL,0,NULL,NULL },
  /* MODE_MENU      */ { "Mode",          SETTINGS,     SEQUENTIAL_OPTION,  OFFSET,             MENU_SUBMENU,        MODE_SEQUENTIAL,    NULL,0,NULL,          NULL,0,NULL,NULL },
  /* OFFSET         */ { "Offset",        SETTINGS,     -1,                 STEPSIZE,           MENU_VALUE_FLOAT,    MODE_SEQUENTIAL,    NULL,0,NULL,          offsetValues,offsetCount,&settings.offsetIndex,NULL },
  /* STEPSIZE       */ { "Stepsize",      SETTINGS,     -1,                 DURATION,           MENU_VALUE_FLOAT,    MODE_SEQUENTIAL,    NULL,0,NULL,          stepsizeValues,stepsizeCount,&settings.stepsizeIndex,stepsizeLabels },
  /* DURATION       */ { "Duration",      SETTINGS,     -1,                 CHANNELS,           MENU_VALUE_FLOAT,    MODE_SEQUENTIAL,    NULL,0,NULL,          durationValues,durationCount,&settings.durationIndex,NULL },
  /* CHANNELS       */ { "Channels",      SETTINGS,     -1,                 SAVE_SETTINGS,      MENU_VALUE_INT,      MODE_SEQUENTIAL,    channelValues,channelCount,&settings.channelIndex, NULL,0,NULL,NULL },
  /* SAVE_SETTINGS  */ { "Save settings", SETTINGS,     -1,                 LOAD_SETTINGS,      MENU_ACTION,         MODE_SEQUENTIAL,    NULL,0,NULL,          NULL,0,NULL,NULL },
  /* LOAD_SETTINGS  */ { "Load settings", SETTINGS,     -1,                 -1,                 MENU_ACTION,         MODE_SEQUENTIAL,    NULL,0,NULL,          NULL,0,NULL,NULL },
  /* SEQ_OPT        */ { "Sequential",    MODE_MENU,    -1,                 SIMULTANEOUS_OPTION,MENU_OPTION,         MODE_SEQUENTIAL,    NULL,0,NULL,          NULL,0,NULL,NULL },
  /* SIM_OPT        */ { "Simultaneous",  MODE_MENU,    -1,                 -1,                 MENU_OPTION,         MODE_SIMULTANEOUS,  NULL,0,NULL,          NULL,0,NULL,NULL },
};
// clang-format on

// --------------------------------------------------
// Initialize menu
// --------------------------------------------------

void initializeMenu() {

  initializeSettings();

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

static void printFloatValue(int item, int index) {

  MenuItem* current = &menu[item];

  if (current->floatLabels != NULL)
    lcd.print(current->floatLabels[index]);
  else
    lcd.print(current->floatValues[index], 2);
}

// --------------------------------------------------

static void printEditValue(int item) {

  MenuItem* current = &menu[item];

  if (current->type == MENU_VALUE_INT &&
      current->intValues != NULL &&
      current->intValueIndex != NULL) {
    lcd.print(current->intValues[*current->intValueIndex]);
  }

  else if (current->type == MENU_VALUE_FLOAT &&
           current->floatValues != NULL &&
           current->floatValueIndex != NULL) {
    printFloatValue(item, *current->floatValueIndex);
  }

  else {
    lcd.print("?");
  }
}

// --------------------------------------------------

static void showValue(int item, int row) {

  MenuItem* current = &menu[item];

  if (item == MODE_MENU) {
    lcd.setCursor(11, row);
    lcd.print(modeNames[settings.mode]);
  }

  if (current->type == MENU_VALUE_INT &&
      current->intValues != NULL &&
      current->intValueIndex != NULL) {
    lcd.setCursor(11, row);
    lcd.print(current->intValues[*current->intValueIndex]);
  }

  if (current->type == MENU_VALUE_FLOAT &&
      current->floatValues != NULL &&
      current->floatValueIndex != NULL) {

    if (current->floatLabels != NULL) {
      lcd.setCursor(11, row);
      lcd.print(current->floatLabels[*current->floatValueIndex]);
    }

    else {
      printValue(current->floatValues[*current->floatValueIndex], 11, row);
    }
  }

  if (current->type == MENU_OPTION && settings.mode == current->option) {
    lcd.setCursor(15, row);
    lcd.print('*');
  }
}

// --------------------------------------------------

static void showInfo() {

  lcd.setCursor(0, 0);
  lcd.print(VERSION_LINE1);

  lcd.setCursor(0, 1);
  lcd.print(VERSION_LINE2);
}

// --------------------------------------------------
// Display
// --------------------------------------------------

void displayMenu() {

  lcd.clear();

  if (showingInfo && currentMenu == INFORMATION) {
    showInfo();
    return;
  }

  // ------------------------------------------------
  // Editing a value
  // ------------------------------------------------

  if (editing) {

    lcd.setCursor(0, 0);
    lcd.print("Edit:");
    lcd.print(menu[selectedItem].label);

    lcd.setCursor(0, 1);
    lcd.print("< ");
    printEditValue(selectedItem);
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

  showingInfo = (item == INFORMATION);

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

    showingInfo = (currentMenu == INFORMATION);

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

  StoredSettings stored;

  stored.magic = EEPROM_MAGIC;
  strncpy(stored.signature, VERSION_LINE2, EEPROM_SIGNATURE_SIZE - 1);
  stored.signature[EEPROM_SIGNATURE_SIZE - 1] = '\0';
  stored.mode = settings.mode;
  stored.offsetIndex = settings.offsetIndex;
  stored.stepsizeIndex = settings.stepsizeIndex;
  stored.durationIndex = settings.durationIndex;
  stored.channelIndex = settings.channelIndex;

  EEPROM.put(EEPROM_ADDRESS, stored);
}

// --------------------------------------------------

bool loadSettings() {

  StoredSettings stored;

  EEPROM.get(EEPROM_ADDRESS, stored);

  if (stored.magic != EEPROM_MAGIC)
    return false;

  if (stored.mode >= MODE_SEQUENTIAL && stored.mode <= MODE_SIMULTANEOUS)
    settings.mode = stored.mode;

  settings.offsetIndex = constrain(stored.offsetIndex, 0, offsetCount - 1);
  settings.stepsizeIndex = constrain(stored.stepsizeIndex, 0, stepsizeCount - 1);
  settings.durationIndex = constrain(stored.durationIndex, 0, durationCount - 1);
  settings.channelIndex = constrain(stored.channelIndex, 0, channelCount - 1);

  return true;
}

// --------------------------------------------------

void initializeSettings() {

  StoredSettings stored;

  EEPROM.get(EEPROM_ADDRESS, stored);

  if (stored.magic != EEPROM_MAGIC ||
      strcmp(stored.signature, VERSION_LINE2) != 0) {

    settings = defaultSettings;
    saveSettings();

    return;
  }

  loadSettings();
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

    MenuItem* current = &menu[selectedItem];
    int* valueIndex = NULL;
    int valueCount = 0;

    if (current->type == MENU_VALUE_INT) {
      valueIndex = current->intValueIndex;
      valueCount = current->intCount;
    }

    else if (current->type == MENU_VALUE_FLOAT) {
      valueIndex = current->floatValueIndex;
      valueCount = current->floatCount;
    }

    if (valueIndex != NULL) {

      if (button == BTN_UP) {

        if (*valueIndex < valueCount - 1)
          (*valueIndex)++;
      }

      if (button == BTN_DOWN) {

        if (*valueIndex > 0)
          (*valueIndex)--;
      }
    }

    if (button == BTN_LEFT || button == BTN_RIGHT) {

      editing = false;
    }

    displayMenu();

    return;
  }

  // ------------------------------------------------
  // Information display
  // ------------------------------------------------

  if (showingInfo && currentMenu == INFORMATION) {

    if (button == BTN_LEFT) {

      goBack();
      displayMenu();

      return;
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

    MenuItem* current = &menu[selectedItem];

    switch (current->type) {

      case MENU_SUBMENU:

        enterMenu(selectedItem);

        break;

      case MENU_VALUE_INT:

        editing = true;

        break;

      case MENU_VALUE_FLOAT:

        editing = true;

        break;

      case MENU_OPTION:

        settings.mode = current->option;

        lcd.clear();
        lcd.print("Mode selected:");

        lcd.setCursor(0, 1);
        lcd.print(current->label);

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
