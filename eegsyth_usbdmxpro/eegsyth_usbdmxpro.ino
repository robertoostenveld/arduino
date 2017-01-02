/*
 * The purpose of this sketch is to implement a module that converts from USB to DMX512. 
 * This allows to use computer software to control stage lighting lamps 
 * 
 * This sketch is (partially) compatible with the Enntec DMX Pro module and software that 
 * is compatible with that module is expected to work with this module as well. 
 * 
 * Components
 * - Arduino Nano or other 5V Arduino board, e.g. http://ebay.to/2iAeUON 
 * - MAX485 module, e.g.  http://ebay.to/2iuKQlr 
 * - 3 or 5 pin female XLR connector
 * 
 * Wiring scheme
 * - connect 3.3V and GND from the Arduino to Vcc and GND of the MAX485 module
 * - connect pin DE (data enable) and RE (receive enable) of the MAX485 module to 3.3V
 * - connect pin D2 of the Arduino to the DI (data in) pin of the MAX485 module
 * - connect pin A to XLR 3 
 * - connect pin B to XLR 2 
 * - connect GND   to XLR 1 
 * 
 */

#include <DmxSimple.h>

#define DI_PIN 2 // data out of the Arduino, data in to the MAX485

#define DMX_PRO_HEADER_SIZE   4
#define DMX_PRO_START_MSG     0x7E
#define DMX_START_CODE        0
#define DMX_START_CODE_SIZE   1
#define DMX_PRO_SEND_PACKET   6 // "periodically send a DMX packet" mode
#define DMX_PRO_END_SIZE      1
#define DMX_PRO_END_MSG       0xE7
#define DMX_PRO_SEND_SIZE_LSB 10
#define DMX_PRO_SEND_SIZE_MSB 11

unsigned char state;
unsigned int dataSize;
unsigned int channel;

void setup() {
  Serial.begin(57600);
  DmxSimple.usePin(DI_PIN);
  state = DMX_PRO_END_MSG;
}


void loop() {
  unsigned char c;

  while (!Serial.available())
    yield();
    
  c = Serial.read();
  if (c == DMX_PRO_START_MSG && state == DMX_PRO_END_MSG) {
    state = c;
  }
  else if (c == DMX_PRO_SEND_PACKET && state == DMX_PRO_START_MSG) {
    state = c;
  }
  else if (state == DMX_PRO_SEND_PACKET ) {
    dataSize = c & 0xff;
    state = DMX_PRO_SEND_SIZE_LSB;
  }
  else if (state == DMX_PRO_SEND_SIZE_LSB) {
    dataSize += (c << 8) & 0xff00;
    state = DMX_PRO_SEND_SIZE_MSB;
  }
  else if ( c == DMX_START_CODE && state == DMX_PRO_SEND_SIZE_MSB) {
    state = c;
    channel = 1;
  }
  else if ( state == DMX_START_CODE && channel < dataSize) {
    DmxSimple.write(channel, c);
    channel++;
  }
  else if ( state == DMX_START_CODE && channel == dataSize && c == DMX_PRO_END_MSG) {
    state = c;
  }
}
