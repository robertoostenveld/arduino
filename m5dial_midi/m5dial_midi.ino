#include <M5Dial.h>
//#include <Control_Surface.h>
#include "colormap.h"

#define clip(x, lo, hi) (x < lo ? lo : (x > hi ? hi : x))

long chan = 0;
const int NCHAN = 16;
long value[NCHAN];
long oldPosition;

//USBMIDI_Interface midi;

enum { CHAN,
       VALUE } mode = CHAN;

void updateMIDI() {
//    MIDIAddress controller = {chan, Channel_1};
//    midi.sendControlChange(controller, value[chan]);
}

void updateScreen() {
  String s;
  if (mode == CHAN) {
    s = String("c ") + String(chan);
  } else {
    s = String("v ") + String(value[chan]);
  }
  Serial.println(s);
  M5Dial.Display.clear();
  M5Dial.Display.drawString(s,
                            M5Dial.Display.width() / 2,
                            M5Dial.Display.height() / 2);
}

void setup() {
  auto cfg = M5.config();
  Control_Surface.begin();  // Initialize the Control Surface
  M5Dial.begin(cfg, true, false);
  M5Dial.Display.setTextColor(GREEN);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);
  M5Dial.Display.setTextSize(1);
  oldPosition = M5Dial.Encoder.read();
  for (int chan = 0; chan < NCHAN; chan++)
    value[chan] = 0;
}

void loop() {
  M5Dial.update();
  Control_Surface.loop();  // Update the Control Surface

  if (M5Dial.BtnA.wasPressed()) {
    if (mode == CHAN)
      mode = VALUE;
    else
      mode = CHAN;
    updateScreen();
  }

  long newPosition = M5Dial.Encoder.read();
  if (newPosition != oldPosition) {
    if (mode == CHAN) {
      chan += (newPosition - oldPosition);
      chan = clip(chan, 0, NCHAN - 1);
    } else {
      value[chan] += (newPosition - oldPosition);
      value[chan] = clip(value[chan], 0, 127);
      updateMIDI();
    }
    updateScreen();
    oldPosition = newPosition;
  }
}