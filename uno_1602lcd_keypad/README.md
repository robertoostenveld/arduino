# Arduino Uno keypad shield with 16x2 LCD 


## Menu structure

Main menu
├── Start
├── Settings
│   ├── Mode
│   │   ├── Single
│   │   ├── Sequential
│   │   └── Simultaneous
│   ├── Offset
│   ├── Stepsize
│   ├── Duration
│   ├── Save settings
│   └── Reset
└── Information
    ├── Settings
    ├── Version
    └── Copyright


## Settings

- Mode is an enum: Single, Sequential or Simultaneous. The active mode is marked with `*` in the Mode menu.
- Offset, Stepsize and Duration are selected from a pre-defined list of floating point values. UP / DOWN cycles through the available values while editing.
- Save settings stores the current mode and value indices in EEPROM.
- Reset reloads the saved settings from EEPROM; if nothing has been saved yet, the current settings are left unchanged.
- Start shows that the sequence has started, Information > Settings shows the current settings.


## Buttons 

UP / DOWN — navigate through menu items or change a value while editing
RIGHT — enter submenu, edit a value, select a mode or perform an action
LEFT — go back one level or stop editing
SELECT — return directly to the main menu (top level)
