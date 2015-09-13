#include <Wire.h>//Include the Wire library to talk I2C

#define cv1    2      // the pin controlling the voltage output
#define cv2    3      // the pin controlling the voltage output
#define cv3    4      // the pin controlling the voltage output
#define cv4    5      // the pin controlling the voltage output
#define gate1 A0      // the pin controlling the digital gate
#define gate2 A1      // the pin controlling the digital gate
#define gate3 A2      // the pin controlling the digital gate
#define gate4 A3      // the pin controlling the digital gate

#define NONE    0
#define VOLTAGE 1
#define GATE    2

//This is the I2C Address of the MCP4725, by default (A0 pulled to GND).
//Please note that this breakout is for the MCP4725A0.
//For devices with A0 pulled HIGH, use 0x61
#define MCP4725_ADDR 0x60

void setup() {
  // initialize the serial communication:
  Serial.begin(115200);
  Serial.print("\n[eegsynth_cvgate / ");
  Serial.print(__DATE__);
  Serial.print(" / ");
  Serial.print(__TIME__);
  Serial.println("]");
  Serial.setTimeout(1000);

  Wire.begin();

  // initialize the gates as an output:
  pinMode(gate1, OUTPUT);
  pinMode(gate2, OUTPUT);
  pinMode(gate3, OUTPUT);
  pinMode(gate4, OUTPUT);
}

void loop() {
  byte b, channel = 0, command = NONE;
  int value = 0;

  if (Serial.available()) {

    // possible sequences of characters are
    // *c1v1024#  control 1 voltage 5*1024/4096 = 1.25 V
    // *g1v1#     gate 1 value ON

    b = Serial.read();
    if (b == '*') {
      Serial.readBytes(&b, 1);
      if (b == 'c') {
        command = VOLTAGE;
        value = 0;
        Serial.readBytes(&b, 1); channel = b - 48; // character '1' is ascii value 49
        Serial.readBytes(&b, 1); // 'v'
        Serial.readBytes(&b, 1); value  = (b - 48) * 1000;
        Serial.readBytes(&b, 1); value += (b - 48) * 100;
        Serial.readBytes(&b, 1); value += (b - 48) * 10;
        Serial.readBytes(&b, 1); value += (b - 48) * 1;
        Serial.readBytes(&b, 1); command = (b == '#' ? command : NONE);
      }
      else if (b == 'g') {
        command = GATE;
        Serial.readBytes(&b, 1); channel = b - 48; // character '1' is ascii value 49
        Serial.readBytes(&b, 1); // 'v'
        Serial.readBytes(&b, 1); value  = (b == '1');
        Serial.readBytes(&b, 1); command = (b == '#' ? command : NONE);
      }
      else {
        command = NONE;
      }
    }
    else {
      command = NONE;
    }

    if (command == VOLTAGE) {
      Serial.print("setting control voltage ");
      Serial.print(channel);
      Serial.print(" to ");
      Serial.println(value);
      Wire.beginTransmission(MCP4725_ADDR);
      Wire.write(64);                      // cmd to update the DAC
      Wire.write((value     ) >> 4);       // the 8 most significant bits...
      Wire.write((value & 15) << 4);       // the 4 least significant bits...
      Wire.endTransmission();


    }
    else if (command == GATE) {
      Serial.print("setting gate ");
      Serial.print(channel);
      Serial.print(" to ");
      Serial.println(value != 0);
      switch (channel) {
        case 1:
          digitalWrite(gate1, value != 0);
          break;
        case 2:
          digitalWrite(gate2, value != 0);
          break;
        case 3:
          digitalWrite(gate3, value != 0);
          break;
        case 4:
          digitalWrite(gate4, value != 0);
          break;
      }
    }
    else {
      Serial.println("invalid command");
    }
  }
}


