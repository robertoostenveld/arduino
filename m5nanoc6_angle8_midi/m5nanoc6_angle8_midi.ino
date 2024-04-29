/*
   This is a sketch for an M5stack NanoC6 controller connected to an 8Angle input module.
   It provides MIDI control change (CC) messages over Bluetooth.

   See https://robertoostenveld.nl/low-cost-8-channel-midi-controller/
*/

#include <M5Unified.h>       // https://github.com/m5stack/M5Unified
#include <M5_ANGLE8.h>       // https://github.com/m5stack/M5Unit-8Angle
#include <Control_Surface.h> // https://github.com/tttapa/Control-Surface
#include "colormap.h"

BluetoothMIDI_Interface midi;
M5_ANGLE8 angle8;

bool sw;
uint16_t value[ANGLE8_TOTAL_ADC];

void updateSwitch(bool sw) {
  uint32_t rgb = 0x0000FF00 * sw; // green or black
  uint8_t brightness = 64;
  angle8.setLEDColor(8, rgb, brightness);
}

void updateValue(bool sw, uint8_t knob, uint8_t value) {
  uint8_t brightness = 64;
  uint32_t rgb = (r[value] << 16) | (g[value] << 8) | (b[value] << 0);
  angle8.setLEDColor(knob, rgb, brightness);

  // the switch allows selecting MIDI channel 1 or 2
  if (sw == 0) {
    MIDIAddress controller = {knob, Channel_1};
    midi.sendControlChange(controller, value);
  }
  else {
    MIDIAddress controller = {knob, Channel_2};
    midi.sendControlChange(controller, value);
  }

  Serial.print(sw);
  Serial.print(" ");
  Serial.print(knob);
  Serial.print(" ");
  Serial.println(value);
}

void setup() {
  M5.begin();
  Control_Surface.begin();

  while (!angle8.begin(ANGLE8_I2C_ADDR)) {
    Serial.println("angle8 connect error");
    delay(100);
  }
  Serial.println("angle8 connect OK");
  Serial.println(angle8.getVersion());

  sw = angle8.getDigitalInput();
  updateSwitch(sw);

  for (uint8_t knob = 0; knob < ANGLE8_TOTAL_ADC; knob++) {
    value[knob] = (255 - angle8.getAnalogInput(knob, _8bit)) / 2;
    updateValue(sw, knob, value[knob]);
  }

  Serial.println("setup done");
}

void loop() {
  M5.update();
  Control_Surface.loop();

  bool newsw = angle8.getDigitalInput();
  if (newsw != sw) {
    updateSwitch(newsw);
    sw = newsw;
  }

  for (uint8_t knob = 0; knob < ANGLE8_TOTAL_ADC; knob++) {
    uint16_t newvalue = (255 - angle8.getAnalogInput(knob, _8bit)) / 2;
    if (value[knob] != newvalue) {
      updateValue(sw, knob, newvalue);
      value[knob] = newvalue;
    }
  }
  
  delay(10);
}
