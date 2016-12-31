/*==============================================================================
  Copyright (c) 2013 Soixante circuits

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  ==============================================================================*/

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
