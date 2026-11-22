# Arduino Uno keypad shield with 16x2 LCD

## Menu structure

Main menu
├── Start
├── Settings
│   ├── Mode
│   │   ├── Sequential
│   │   └── Simultaneous
│   ├── Offset
│   ├── Stepsize
│   ├── Channels
│   ├── Duration
│   ├── Loop
│   ├── Save settings
│   └── Load settings
└── Information

## Settings

- Mode is an enum: Sequential or Simultaneous. The active mode is shown as Seq or Sim next to Mode in the Settings menu and marked with `*` in the Mode menu.
- Offset, Stepsize and Duration are selected from a pre-defined list of floating point values. UP / DOWN cycles through the available values while editing. Stepsize is displayed as 1/N.
- Channels selects an integer from 1 to 24.
- Loop is a boolean shown as yes/no; UP / DOWN toggles it while editing.
- Save settings stores the current mode, loop flag and value indices in EEPROM. After a firmware update, default settings are written to EEPROM automatically.
- Load settings gets the saved settings from EEPROM; if nothing has been saved yet, the current settings are left unchanged.
- Start shows that the sequence has started.
- Information shows the version and the compilation date and time. LEFT returns to the main menu.

## Buttons

UP / DOWN — navigate through menu items or change a value while editing
RIGHT — enter submenu, edit a value, select a mode or perform an action
LEFT — go back one level or stop editing
SELECT — return directly to the main menu (top level)
