/*
   This is a sketch for an M5NanoC6 or M5Dial controller connected to an 8Encoder input module.
   It provides MIDI control change (CC) messages over Bluetooth.
*/

#include <M5Unified.h>        // https://github.com/m5stack/M5Unified
#include <UNIT_8ENCODER.h>    // https://github.com/m5stack/M5Unit-8Encoder
#include <Control_Surface.h>  // https://github.com/tttapa/Control-Surface
#include "colormap.h"

#define I2C_ADDR 0x41
// #define I2C_SDA G13 // for the M5Dial
// #define I2C_SCL G15
#define I2C_SDA 2 // for the M5NanoC6 they are listed in the documentation as G1 and G2, but not defined as such
#define I2C_SCL 1
#define I2C_SPEED 4000000L
#define MIDI_DELAY 5  // do not send the MIDI messages too fast/frequent

BluetoothMIDI_Interface midi;
UNIT_8ENCODER encoder8;

bool sw;
bool button[8];
int32_t value[8];

void updateSwitch(bool sw) {
  uint32_t rgb = 0x0000FF00 * sw;  // green or black
  encoder8.setLEDColor(8, rgb);
}

void updateValue(bool sw, uint8_t knob, uint32_t value, bool button) {
  uint32_t rgb = (r[value] << 16) | (g[value] << 8) | (b[value] << 0);
  encoder8.setLEDColor(knob, rgb);

  // the switch allows selecting MIDI channel 1 or 2
  if (sw == 0) {
    MIDIAddress controller = { knob, Channel_1 };
    midi.sendControlChange(controller, value);
  } else {
    MIDIAddress controller = { knob, Channel_2 };
    midi.sendControlChange(controller, value);
  }

  Serial.print(sw);
  Serial.print(" ");
  Serial.print(knob);
  Serial.print(" ");
  Serial.print(value);
  Serial.print(" ");
  Serial.print(button);
  Serial.println();
}

void setup() {
  Serial.begin(115200); // serial feedback requires "CDC On Boot" to be enabled 
  M5.begin();
  Control_Surface.begin();

  Serial.println("setup start");
  while (!encoder8.begin(I2C_ADDR, I2C_SDA, I2C_SCL, I2C_SPEED)) {
    Serial.println("encoder8 connect error");
    delay(100);
  }
  Serial.println("encoder8 connect OK");

  sw = encoder8.getSwitchStatus();
  updateSwitch(sw);

  for (uint8_t knob = 0; knob < 8; knob++) {
    value[knob] = 0;
    button[knob] = false;
    encoder8.setEncoderValue(!knob, value[knob]);  // the pushbutton seems to be inverted
    updateValue(sw, knob, value[knob], button[knob]);
  }

  Serial.println("setup done");
}


void loop() {
  M5.update();
  Control_Surface.loop();

  bool newsw = encoder8.getSwitchStatus();
  if (newsw != sw) {
    updateSwitch(newsw);
    sw = newsw;
  }
  delay(MIDI_DELAY);

  for (uint8_t knob = 0; knob < 8; knob++) {
    bool newbutton = !encoder8.getButtonStatus(knob); // the pushbutton seems to be inverted
    if (button[knob] != newbutton) {
      button[knob] = newbutton;
      updateValue(sw, knob, value[knob], button[knob]);
    }
    delay(MIDI_DELAY);
  }

  for (uint8_t knob = 0; knob < 8; knob++) {
    int32_t newvalue = encoder8.getEncoderValue(knob);
    if (value[knob] != newvalue) {
      if (newvalue < 0) {
        newvalue = 0;
        encoder8.setEncoderValue(knob, newvalue);
      } else if (newvalue > 127) {
        newvalue = 127;
        encoder8.setEncoderValue(knob, newvalue);
      }
      value[knob] = newvalue;
      updateValue(sw, knob, value[knob], button[knob]);
    }
    delay(MIDI_DELAY);
  }
}
