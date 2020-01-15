/*
* EEGSynth Teensy based CV/Gate controller. This sketch allows
* one to use control voltages and gates to interface a computer
* with an analog synthesizer. The hardware comprises a Teensy 
* with two MCP4725 12-bit DAC breakout boards.
*
* Some example sequences of characters are
*   *c1v1024#  control 1 voltage 5*1024/4095 = 1.25 V
*   *g1v1#     gate 1 value ON
*
* This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License.
* See http://creativecommons.org/licenses/by-sa/4.0/
*
* Copyright (C) 2020, Robert Oostenveld, http://www.eegsynth.org/
*/

#include <Wire.h>//Include the Wire library to talk I2C
#include <Adafruit_NeoPixel.h>

#include "colormap.h"

#define enable1 true
#define enable2 true

// 0x60 is the I2C Address of the MCP4725, by default (A0 pulled to GND).
// Please note that this breakout is for the MCP4725A0.
// For devices with A0 pulled HIGH, use 0x61
#define address1 0x60
#define address2 0x61

#define gate1pin A0      // the pin controlling the digital gate
#define gate2pin A1      // the pin controlling the digital gate

#define MCP4726_CMD_WRITEDAC            (0x40)  // Writes data to the DAC
#define MCP4726_CMD_WRITEDACEEPROM      (0x60)  // Writes data to the DAC and the EEPROM (persisting the assigned value after reset)

// the values of the DAC range from 0 to 4095 (12 bits)
#define MAXVALUE  4095.

// these are the commands over the serial interface
#define NONE      0
#define VOLTAGE   1
#define GATE      2

// status after parsing the commands
#define OK     0
#define ERROR -1

#define PIXELPIN 20
#define NUMPIXELS 4
#define BRIGHTNESS 0.3

Adafruit_NeoPixel pixels(NUMPIXELS, PIXELPIN, NEO_GRB + NEO_KHZ800);

// the initial values of the output control voltages and gates
int voltage1 = 0, voltage2 = 0;
int gate1 = 0, gate2 = 0;

void setColor(int led, float value) {
  byte r, g, b;
  int index = value*255.;
  index = (index > 255 ? 255 : index);  // must be between 0 and 255
  r = 255*R[index]*BRIGHTNESS;
  g = 255*G[index]*BRIGHTNESS;
  b = 255*B[index]*BRIGHTNESS;
  pixels.setPixelColor(led, pixels.Color(r, g, b));
  pixels.show();
  return;
}

void setValue(uint16_t address, uint16_t value) {
  uint8_t msb, lsb;
  msb = (value / 16);
  lsb = (value % 16) << 4;
  Wire.beginTransmission(address);
  Wire.write(MCP4726_CMD_WRITEDAC);   // cmd to update the DAC
  Wire.write(msb);                    // the 8 most significant bits...
  Wire.write(lsb);                    // the 4 least significant bits...
  Wire.endTransmission();
  return;
}

void setup() {
  // initialize the serial communication:
  while (!Serial) {;}
  Serial.begin(115200);
  Serial.print("\n[teensy_cvgate_mcp4725_neopixel/ ");
  Serial.print(__DATE__);
  Serial.print(" / ");
  Serial.print(__TIME__);
  Serial.println("]");
  Serial.setTimeout(1000);

  Wire.begin();

  // initialize the gate pins as output:
  pinMode(gate1pin, OUTPUT);
  pinMode(gate2pin, OUTPUT);

  // INITIALIZE NeoPixel strip object
  pixels.begin(); 
  
  // Set all pixel colors to 'off'
  pixels.clear(); 
 
  return;
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
          voltage1 = (value > MAXVALUE ? MAXVALUE : value);
          status = (enable1 ? OK : ERROR);
          break;
        case 2:
          voltage2 = (value > MAXVALUE ? MAXVALUE : value);
          status = (enable2 ? OK : ERROR);
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
    // refresh the output voltages and gates
    if (enable1) {
      setValue(address1, voltage1);
      digitalWrite(gate1pin, gate1);
    }
    if (enable2) {
      setValue(address2, voltage2);
      digitalWrite(gate2pin, gate2);
    }

    // update the color of the Neopixels
    // the integer representation of the control voltage is represented between 0 and 4095
    setColor(0, voltage1/MAXVALUE);
    setColor(1, voltage2/MAXVALUE); 
    // the integer representation of the gate voltage is either 0 or 1
    setColor(2, gate1); 
    setColor(3, gate2); 
  }
} //main
