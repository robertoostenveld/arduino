/*
   This is a sketch for an M5Dial controller connected to an 8Rotary input module.
   It provides MIDI control change (CC) messages over Bluetooth.
*/

#include <M5NanoC6.h>
#include "UNIT_8ENCODER.h"

UNIT_8ENCODER encoder8;

int32_t encoder[8];
bool button[8];
bool sw;

void sendValue(uint8_t sw, uint8_t knob, int32_t encoder, int32_t button) {
  Serial.print(sw);
  Serial.print(" ");
  Serial.print(knob);
  Serial.print(" ");
  Serial.println(encoder);
  Serial.print(" ");
  Serial.println(button);
}

void setup() {
  Serial.begin(115200);
  Serial.println("setup start");
  delay(1000);

//  encoder8.begin(&Wire, ENCODER_ADDR, 2, 1, 100000UL);
  Serial.println("encoder8 connect OK");
  delay(1000);

  Serial.println("setup done");
}

void loop() {
  Serial.println(millis());
  delay(1000);

  /*
    sw = encoder8.getSwitchStatus();
    for (uint8_t knob = 0; knob < 1; knob++) {
    encoder[knob] = encoder8.getEncoderValue(knob);
    button[knob] = encoder8.getButtonStatus(knob);
    sendValue(sw, knob, encoder[knob], button[knob]);
    }

    delay(100);
  */
}
