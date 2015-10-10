/*
* EEGSynth Arduino based CV/Gate controller. This sketch allows
* one to use control voltages and gates to interface a computer
* through an Arduino with an analog synthesizer. The hardware
* comprises an Arduino Nano v3.0 with one or multiple MCP4822
* 12-bit DACs.
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

#include <SPI.h>

const int GAIN_1 = 0x1;
const int GAIN_2 = 0x0;

#define voltage12cs     2      // the slave-select pin for channel 1 and 2 on DAC #1
#define voltage34cs     3      // the slave-select pin for channel 3 and 4 on DAC #2
#define gate1pin A0            // the pin controlling the digital gate
#define gate2pin A1            // the pin controlling the digital gate
#define gate3pin A2            // the pin controlling the digital gate
#define gate4pin A3            // the pin controlling the digital gate

#define NONE    0
#define VOLTAGE 1
#define GATE    2

#define OK    1
#define ERROR 2


// these remember the state of all CV and gate outputs
int voltage1 = 0, voltage2 = 0, voltage3 = 0, voltage4 = 0;
int gate1 = 0, gate2 = 0, gate3 = 0, gate4 = 0;
const int enable1 = 1, enable2 = 1, enable3 = 1, enable4 = 1;

void setDacOutput(byte channel, byte gain, byte shutdown, unsigned int val)
{
  byte lowByte = val & 0xff;
  byte highByte = ((val >> 8) & 0xff) | channel << 7 | gain << 5 | shutdown << 4;

  // note that we are not using the default pin 10 for slave select, see further down
  // PORTB &= 0xfb; // toggle pin 10 as Slave Select low
  SPI.transfer(highByte);
  SPI.transfer(lowByte);
  // PORTB |= 0x4; // toggle pin 10 as Slave Select high
}

void setup() {
  // initialize the serial communication:
  Serial.begin(115200);
  Serial.print("\n[eegsynth_cvgate_mcp4822 / ");
  Serial.print(__DATE__);
  Serial.print(" / ");
  Serial.print(__TIME__);
  Serial.println("]");
  Serial.setTimeout(1000);

  SPI.begin();

  // initialize the gate pins as output:
  pinMode(gate1pin, OUTPUT);
  pinMode(gate2pin, OUTPUT);
  pinMode(gate3pin, OUTPUT);
  pinMode(gate4pin, OUTPUT);

  // initialize the control voltage pins as output:
  pinMode(voltage12cs, OUTPUT);
  pinMode(voltage34cs, OUTPUT);

  // default is to disable the communication with all SPI devices
  digitalWrite(voltage12cs, HIGH);
  digitalWrite(voltage34cs, HIGH);
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
          status = (enable1 ? OK : ERROR);
          break;
        case 2:
          voltage2 = value;
          status = (enable2 ? OK : ERROR);
          break;
        case 3:
          voltage3 = value;
          status = (enable3 ? OK : ERROR);
          break;
        case 4:
          voltage4 = value;
          status = (enable4 ? OK : ERROR);
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
    // refresh all enabled output channels
    if (enable1) {
      digitalWrite(voltage12cs, LOW); // enable slave select
      setDacOutput(0, GAIN_2, 1, voltage1);
      digitalWrite(voltage12cs, HIGH); // disable slave select
      digitalWrite(gate1pin, gate1);
    }
    if (enable2) {
      digitalWrite(voltage12cs, LOW); // enable slave select
      setDacOutput(1, GAIN_2, 1, voltage2);
      digitalWrite(voltage12cs, HIGH); // disable slave select
      digitalWrite(gate2pin, gate2);
    }
    if (enable3) {
      digitalWrite(voltage34cs, LOW); // enable slave select
      setDacOutput(0, GAIN_2, 1, voltage3);
      digitalWrite(voltage34cs, HIGH); // disable slave select
      digitalWrite(gate3pin, gate3);
    }
    if (enable4) {
      digitalWrite(voltage34cs, LOW); // enable slave select
      setDacOutput(1, GAIN_2, 1, voltage4);
      digitalWrite(voltage34cs, HIGH); // disable slave select
      digitalWrite(gate4pin, gate4);
    }
  }
} //main

