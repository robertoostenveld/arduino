/*
 * EEGSynth Arduino based CV/Gate controller. This sketch allows
 * one to use control voltages and gates to interface a computer
 * through an Arduino with an analog synthesizer. The hardware 
 * comprises an Arduino Nano v3.0 with a MCP4725 12-bit DAC.
 * Optionally it can be extended with a number of sample-and-hold
 * ICs, a step-up voltage converter and some opamps.
 *
 * Some example sequences of characters are
 *   *c1v1024#  control 1 voltage 5*1024/4095 = 1.25 V
 *   *g1v1#     gate 1 value ON
 *
 * This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License.
 * See http://creativecommons.org/licenses/by-sa/4.0/ 
 *
 * Copyright (C) 2015, Robert Oostenveld, http://www.eegsynth.org/
 */

#include <Wire.h>//Include the Wire library to talk I2C

#define voltage1pin    2      // the pin controlling the voltage output
#define voltage2pin    3      // the pin controlling the voltage output
#define voltage3pin    4      // the pin controlling the voltage output
#define voltage4pin    5      // the pin controlling the voltage output
#define gate1pin A0      // the pin controlling the digital gate
#define gate2pin A1      // the pin controlling the digital gate
#define gate3pin A2      // the pin controlling the digital gate
#define gate4pin A3      // the pin controlling the digital gate

#define MCP4726_CMD_WRITEDAC            (0x40)  // Writes data to the DAC
#define MCP4726_CMD_WRITEDACEEPROM      (0x60)  // Writes data to the DAC and the EEPROM (persisting the assigned value after reset)

#define NONE    0
#define VOLTAGE 1
#define GATE    2

#define OK    1
#define ERROR 2

//This is the I2C Address of the MCP4725, by default (A0 pulled to GND).
//Please note that this breakout is for the MCP4725A0.
//For devices with A0 pulled HIGH, use 0x61
#define MCP4725_ADDR 0x60

// these remember the state of all CV and gate outputs
int voltage1 = 0, voltage2 = 0, voltage3 = 0, voltage4 = 0;
int gate1 = 0, gate2 = 0, gate3 = 0, gate4 = 0;

void sampleAndHold(int pin, uint16_t value) {
  uint8_t msb, lsb;
  msb = (value / 16);
  lsb = (value % 16) << 4;
  Wire.beginTransmission(MCP4725_ADDR);
  Wire.write(MCP4726_CMD_WRITEDAC);   // cmd to update the DAC
  Wire.write(msb);                    // the 8 most significant bits...
  Wire.write(lsb);                    // the 4 least significant bits...
  Wire.endTransmission();
  digitalWrite(pin, HIGH);
  delay(1);                           // give it some time to Sample and Hold
  digitalWrite(pin, LOW);
  return;
}

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

  // initialize the gate pins as output:
  pinMode(gate1pin, OUTPUT);
  pinMode(gate2pin, OUTPUT);
  pinMode(gate3pin, OUTPUT);
  pinMode(gate4pin, OUTPUT);

  // initialize the control voltage pins as output:
  pinMode(voltage1pin, OUTPUT);
  pinMode(voltage2pin, OUTPUT);
  pinMode(voltage3pin, OUTPUT);
  pinMode(voltage4pin, OUTPUT);
}

void loop() {
  byte b, channel = 0, command = NONE, status = OK;
  int value = 0;

  if (Serial.available()) {

    // parse the input over the serial connection
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

    // update the internal state of all output channels
    if (command == VOLTAGE) {
      switch (channel) {
        case 1:
          voltage1 = value;
          status = OK;
          break;
        case 2:
          voltage2 = value;
          status = OK;
          break;
        case 3:
          voltage3 = value;
          status = OK;
          break;
        case 4:
          voltage4 = value;
          status = OK;
          break;
        default:
          status = ERROR;
      }

    }
    else if (command == GATE) {
      switch (channel) {
        case 1:
          gate1 = (value != 0);
          status = OK;
          break;
        case 2:
          gate2 = (value != 0);
          status = OK;
          break;
        case 3:
          gate3 = (value != 0);
          status = OK;
          break;
        case 4:
          gate4 = (value != 0);
          status = OK;
          break;
        default:
          status = ERROR;
      }
    }
    else {
      status = ERROR;
    }
    if (status == OK)
      Serial.println("ok");
    else if (status == ERROR)
      Serial.println("error");
  }
  else {
    // refresh all output channels
    sampleAndHold(voltage1pin, voltage1);
    sampleAndHold(voltage2pin, voltage2);
    sampleAndHold(voltage3pin, voltage3);
    sampleAndHold(voltage4pin, voltage4);
    digitalWrite(gate1pin, gate1);
    digitalWrite(gate2pin, gate2);
    digitalWrite(gate3pin, gate3);
    digitalWrite(gate4pin, gate4);
  }
} //main


