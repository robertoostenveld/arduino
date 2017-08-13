/*
   The purpose of this sketch is to implement a module that converts from USB to DMX512.
   This allows to use computer software to control stage lighting lamps

   This sketch is partially compatible with the Enttec DMX USB Pro module, but the startup
   sequence of this Arduino sketch is different and other/commercial software is likely not
   to recognize the module as DMX USB Pro .

   Components
   - Arduino Nano or compatible 5V board, e.g. http://ebay.to/2iAeUON
   - MAX485 module, e.g.  http://ebay.to/2iuKQlr
   - 3 or 5 pin female XLR connector

   Wiring scheme
   - connect 3.3V and GND from the Arduino to Vcc and GND of the MAX485 module
   - connect pin DE (data enable) and RE (receive enable) of the MAX485 module to 3.3V
   - connect pin D2 of the Arduino to the DI (data in) pin of the MAX485 module
   - connect pin A to XLR 3
   - connect pin B to XLR 2
   - connect GND   to XLR 1

*/

#include <DmxSimple.h>

#define DI_PIN 2 // data out of the Arduino, data in to the MAX485

// these are defined in https://www.enttec.com/docs/dmx_usb_pro_api_spec.pdf
#define DMX_PRO_START_MSG             126  // 0x7E
#define DMX_PRO_END_MSG               231  // 0xE7

// the packets can have the following labels, i.e. content
#define DMX_PRO_REPROGRAM_FIRMWARE    1
#define DMX_PRO_FLASH_PAGE            2
#define DMX_PRO_GET_WIDGET_PARAM      3
#define DMX_PRO_SET_WIDGET_PARAM      4
#define DMX_PRO_RECEIVED_DMX          5
#define DMX_PRO_SEND_DMX              6
#define DMX_PRO_SEND_RDM              7
#define DMX_PRO_RECEIVE_DMX_ON_CHANGE 8
#define DMX_PRO_RECEIVED_DMX_CHANGE   9
#define DMX_PRO_GET_SERIAL_NUMBER     10
#define DMX_PRO_SENT_DRM_DISCOVERY    11

// each DMX data packet starts with this code
#define DMX_PRO_START_CODE            0

// these are some local states
#define DMX_PRO_SEND_DMX_LSB          240
#define DMX_PRO_SEND_DMX_MSB          241
#define DMX_PRO_SEND_DMX_DATA         242

unsigned char state = DMX_PRO_END_MSG;
unsigned int dataSize;
unsigned int channel;

void setup() {
  DmxSimple.usePin(DI_PIN);
  Serial.begin(57600);
  while (!Serial);
}

void loop() {
  while (!Serial.available())
    yield();

  unsigned char c = Serial.read();

  if (c == DMX_PRO_START_MSG && state != DMX_PRO_END_MSG) {
    state = c; // this should not happen, it means that part of the previous packet was lost
  }
  if (c == DMX_PRO_START_MSG && state == DMX_PRO_END_MSG) {
    state = c;
  }
  else if (c == DMX_PRO_SEND_DMX && state == DMX_PRO_START_MSG) {
    state = c;
  }
  else if (state == DMX_PRO_SEND_DMX) {
    dataSize = c & 0xff;
    state = DMX_PRO_SEND_DMX_LSB;
  }
  else if (state == DMX_PRO_SEND_DMX_LSB) {
    dataSize += (c << 8) & 0xff00;
    state = DMX_PRO_SEND_DMX_MSB;
  }
  else if (state == DMX_PRO_SEND_DMX_MSB && c == DMX_PRO_START_CODE) {
    state = DMX_PRO_SEND_DMX_DATA;
    channel = 1;
  }
  else if (state == DMX_PRO_SEND_DMX_DATA && channel < dataSize) {
    DmxSimple.write(channel, c);
    channel++;
  }
  else if (state == DMX_PRO_SEND_DMX_DATA && channel == dataSize && c == DMX_PRO_END_MSG) {
    state = c;
  }
}
