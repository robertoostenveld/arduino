// ethernet starts off from http://arduino.cc/en/Tutorial/DhcpAddressPrinter
// thingspeak starts off from http://community.thingspeak.com/arduino/ThingSpeakClient.pde

#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <MemoryFree.h>
#include "secret.h"

#ifdef DEBUG
#define DEBUG_PRINT(x)     Serial.print (x)
#define DEBUG_PRINT(x, y)  Serial.print (x, y)
#define DEBUG_PRINTLN(x)   Serial.println (x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINT2(x, y)
#define DEBUG_PRINTLN(x) 
#endif

byte mac[] = {  
  0x90, 0xA2, 0xDA, 0x0D, 0x2A, 0x4A };

byte buf[32]; // ring buffer
unsigned int bufptr = 0; // pointer into ring buffer
unsigned int bufblk = 0; // boolean to block buffer updates

// ThingSpeak Settings
IPAddress server(184, 106, 153, 149 );          // IP Address for the ThingSpeak API
EthernetClient client;

/*
 * The following are defined in secret.h
 *
 #define writeAPIKey1 "XXXXXXXXXXXXXXXX" // Write API Key for a ThingSpeak Channel
 #define writeAPIKey2 "XXXXXXXXXXXXXXXX" // Write API Key for a ThingSpeak Channel
 #define writeAPIKey3 "XXXXXXXXXXXXXXXX" // Write API Key for a ThingSpeak Channel
 */

const unsigned long updateInterval = 30000;  // Time interval in milliseconds to update ThingSpeak, see https://thingspeak.com/docs/channels#rate_limits

typedef struct message_t {
  unsigned long id;
  unsigned long counter;
  float value1;
  float value2;
  float value3;
  float value4;
  float value5;
  unsigned long crc;
};

// Variable Setup
unsigned long lastConnectionTime = 0; 
boolean lastConnectionStatus = false;
unsigned int lastChannel = 1; // this should start at 1, 2 or 3
unsigned int resetCounter = 0;

// the receiving rmf12b module itself is number 1, which is why it is not listed here 
message_t message2, message3, message4, message5, message6, message7;
boolean update2 = false, update3 = false, update4 = false, update5 = false, update6 = false, update7 = false;

/*******************************************************************************************************************/

void setup() {
  Serial.begin(57600);
  Serial.println(F("\n[rfm12b_thingspeak / " __DATE__ " / " __TIME__ "]"));

  // start the Ethernet connection:
  while (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP"));
    delay(5000);
  }
  // print your local IP address:
  Serial.print(F("My IP address: "));
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print("."); 
  }
  Serial.println();

  for (int i; i<32; i++) {
    buf[i] = 0;
  }

  Wire.begin(9);                // Start I2C Bus as a Slave (Device Number 9)
  Wire.onReceive(receiveEvent); // register event

} // setup

/*******************************************************************************************************************/

void loop() {

  // Print Update Response to Serial Monitor
  if (client.available())
  {
    char c = client.read();
    DEBUG_PRINT(c);
  }

  // Disconnect from ThingSpeak
  if (!client.connected() && lastConnectionStatus)
  {
    DEBUG_PRINTLN();
    client.stop();
  }


  if (bufblk) {
    // the I2C buffer is full and needs to be processed
    message_t *message = (message_t *)buf;

    // DEBUG_PRINT("freeMemory = ");
    // DEBUG_PRINTLN(freeMemory());
    // DEBUG_PRINT("interval = ");
    // DEBUG_PRINTLN(millis() - lastConnectionTime);

    unsigned long check = crc_buf((char *)message, sizeof(message_t) - sizeof(unsigned long));

    if (message->crc==check) {

      DEBUG_PRINT(message->id);
      DEBUG_PRINT(",\t");
      DEBUG_PRINT(message->counter);
      DEBUG_PRINT(",\t");
      DEBUG_PRINT2(message->value1, 2);
      DEBUG_PRINT(",\t");
      DEBUG_PRINT2(message->value2, 2);
      DEBUG_PRINT(",\t");
      DEBUG_PRINT2(message->value3, 2);
      DEBUG_PRINT(",\t");
      DEBUG_PRINT2(message->value4, 2);
      DEBUG_PRINT(",\t");
      DEBUG_PRINT2(message->value5, 2);
      DEBUG_PRINT(",\t");
      DEBUG_PRINTLN(message->crc);

      // store the received message
      if (!client.connected()) {
        switch (message->id) {
        case 2:
          memcpy(&message2, message, sizeof(message_t));
          update2 = true;
          break;
        case 3:
          memcpy(&message3, message, sizeof(message_t));
          update3 = true;
          break;
        case 4:
          memcpy(&message4, message, sizeof(message_t));
          update4 = true;
          break;
        case 5:
          memcpy(&message5, message, sizeof(message_t));
          update5 = true;
          break;
        case 6:
          memcpy(&message6, message, sizeof(message_t));
          update6 = true;
          break;
        case 7:
          memcpy(&message7, message, sizeof(message_t));
          update7 = true;
          break;
        } // switch

      }
      bufblk = 0; // release buffer
    }
    else {
      DEBUG_PRINTLN(F("CRC mismatch"));
    }
  }

  // update ThingSpeak
  if (!client.connected() && (millis() - lastConnectionTime > updateInterval)) {

    unsigned int thisChannel = 0;

    // decide which channel should be updated
    // this uses a round-robin scheme, starting from the next channel in line

    switch (lastChannel) {
    case 1:
      if (update5 || update6)
        thisChannel = 2;
      else if (update7)
        thisChannel = 3;
      else if (update2 || update3 || update4)
        thisChannel = 1;
      break;

    case 2:
      if (update7)
        thisChannel = 3;
      else if (update2 || update3 || update4)
        thisChannel = 1;
      else if (update5 || update6)
        thisChannel = 2;
      break;

    case 3:
      if (update2 || update3 || update4)
        thisChannel = 1;
      else if (update5 || update6)
        thisChannel = 2;
      else if (update7)
        thisChannel = 3;
      break;
    } //switch    

    String postString;

    DEBUG_PRINT("update2 = "); 
    DEBUG_PRINTLN(update2);
    DEBUG_PRINT("update3 = "); 
    DEBUG_PRINTLN(update3);
    DEBUG_PRINT("update4 = "); 
    DEBUG_PRINTLN(update4);
    DEBUG_PRINT("update5 = "); 
    DEBUG_PRINTLN(update5);
    DEBUG_PRINT("update6 = "); 
    DEBUG_PRINTLN(update6);
    DEBUG_PRINT("update7 = "); 
    DEBUG_PRINTLN(update7);
    DEBUG_PRINT("lastChannel = "); 
    DEBUG_PRINTLN(lastChannel);
    DEBUG_PRINT("thisChannel = "); 
    DEBUG_PRINTLN(thisChannel);

    if (thisChannel==1 && update2) {
      // LM35: V, T
      postString += "&field1="+String(message2.value1)+"&field2="+String(message2.value2);
      update2 = 0;
    }
    if (thisChannel==1 && update3) {
      // AM2301: V, T, H
      postString += "&field3="+String(message3.value1)+"&field4="+String(message3.value2)+"&field5="+String(message3.value3);
      update3 = false;
    }
    if (thisChannel==1 && update4) {
      // BMP085: V, T, P, NOTE field 7 and 8 are swapped in the ThingSpeak channel
      postString += "&field6="+String(message4.value1)+"&field8="+String(message4.value2)+"&field7="+String(message4.value3);
      update4 = false;
    }
    if (thisChannel==2 && update5) {
      // 2x CNY50: V, KWh, M3
      postString += "&field1="+String(message5.value1)+"&field2="+String(message5.value2)+"&field3="+String(message5.value3);
      update5 = false;
    }
    if (thisChannel==2 && update6) {
      // 2x DS18B20: V, T, T
      postString += "&field4="+String(message6.value1)+"&field5="+String(message6.value2)+"&field6="+String(message6.value3);
      update6 = false;
    }
    if (thisChannel==3 && update7) {
      // BMP180 with ESP8266: V, T, P
      postString += "&field1="+String(message7.value1)+"&field2="+String(message7.value2)+"&field3="+String(message7.value3);
      update7 = false;
    }

    if (thisChannel==1) {
      updateThingSpeak(postString, writeAPIKey1);
      lastChannel = thisChannel;
    }
    if (thisChannel==2) {
      updateThingSpeak(postString, writeAPIKey2);
      lastChannel = thisChannel;
    }
    if (thisChannel==3) {
      updateThingSpeak(postString, writeAPIKey3);
      lastChannel = thisChannel;
    }

    lastConnectionTime = millis();
  }

  lastConnectionStatus = client.connected();
} //loop

/*******************************************************************************************************************/

void updateThingSpeak(const String tsData, const String writeAPIKey)
{
  if (client.connect(server, 80))
  { 
    DEBUG_PRINTLN(F("Connected to ThingSpeak..."));
    DEBUG_PRINTLN();

    client.print(F("POST /update HTTP/1.1\n"));
    client.print(F("Host: api.thingspeak.com\n"));
    client.print(F("Connection: close\n"));
    client.print(F("X-THINGSPEAKAPIKEY: "));
    client.print(writeAPIKey);
    client.print(F("\n"));
    client.print(F("Content-Type: application/x-www-form-urlencoded\n"));
    client.print(F("Content-Length: "));
    client.print(tsData.length());
    client.print(F("\n\n"));
    client.print(tsData);

    resetCounter = 0;
  }
  else
  {
    DEBUG_PRINTLN(F("Connection Failed."));   
    DEBUG_PRINTLN();

    resetCounter++;

    if (resetCounter >=5 ) {
      resetEthernetShield();
    }
  }
} // updateThingSpeak

void resetEthernetShield()
{
  Serial.println(F("Resetting Ethernet Shield."));   
  Serial.println();

  client.stop();
  delay(1000);

  while (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP"));
    delay(5000);
  }
}

void receiveEvent(int howMany) {
  while (howMany-->0) {
    int x = Wire.read(); // receive byte as an integer
    if (!bufblk)
      buf[bufptr++] = x; // insert into buffer

    if ((bufptr % 32)==0) {
      bufptr = 0; // point to the first element
      bufblk = 1; // block buffer
    }
  }

} // receiveEvent

/*******************************************************************************************************************/

static PROGMEM prog_uint32_t crc_table[16] = {
  0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
  0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
  0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
  0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

unsigned long crc_update(unsigned long crc, byte data)
{
  byte tbl_idx;
  tbl_idx = crc ^ (data >> (0 * 4));
  crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
  tbl_idx = crc ^ (data >> (1 * 4));
  crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
  return crc;
}

unsigned long crc_buf(char *b, long l)
{
  unsigned long crc = ~0L;
  for (unsigned long i=0; i<l; i++)
    crc = crc_update(crc, ((char *)b)[i]);
  crc = ~crc;
  return crc;
}

unsigned long crc_string(char *s)
{
  unsigned long crc = ~0L;
  while (*s)
    crc = crc_update(crc, *s++);
  crc = ~crc;
  return crc;
}



