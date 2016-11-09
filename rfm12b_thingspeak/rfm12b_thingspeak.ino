// ethernet starts off from http://arduino.cc/en/Tutorial/DhcpAddressPrinter
// thingspeak starts off from http://community.thingspeak.com/arduino/ThingSpeakClient.pde

#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <String.h>
#include <Syslog.h>
#include <MemoryFree.h>
#include "secret.h"

#define DEBUG_PRINT(x)     Serial.print (x)
#define DEBUG_PRINT2(x, y) Serial.print (x, y)
#define DEBUG_PRINTLN(x)   Serial.println (x)


/*
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINT2(x, y)
  #define DEBUG_PRINTLN(x)
*/

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x2A, 0x4A };
byte loghost[] = { 192, 168, 1, 2 };

byte buf[32]; // ring buffer
unsigned int bufptr = 0; // pointer into ring buffer
unsigned int bufblk = 0; // boolean to block buffer updates

// ThingSpeak Settings
IPAddress server(184, 106, 153, 149 );          // IP Address for the ThingSpeak API
EthernetClient client;

/*
    The write API keys for ThingSpeak are defined in secret.h like this

   #define APIKeyChannel2 "XXXXXXXXXXXXXXXX"
   #define APIKeyChannel3 "XXXXXXXXXXXXXXXX"
   #define APIKeyChannel4 "XXXXXXXXXXXXXXXX"
   #define APIKeyChannel5 "XXXXXXXXXXXXXXXX"
   #define APIKeyChannel6 "XXXXXXXXXXXXXXXX"

*/

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
unsigned int resetCounter = 0;

/*******************************************************************************************************************/

void(* resetArduino) (void) = 0; // declare reset function at address 0

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

  for (int i; i < 32; i++) {
    buf[i] = 0;
  }

  Wire.begin(9);                // Start I2C Bus as a Slave (Device Number 9)
  Wire.onReceive(receiveEvent); // register event

  Syslog.setLoghost(loghost);
  Syslog.logger(1, 5, "", "rfm12b_thingspeak:", "setup finished");

} // setup

/*******************************************************************************************************************/

void loop() {

  char stringbuf[256];
  
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

  // Clock rollover, reset after 49 days
  if (millis() < lastConnectionTime)
    resetArduino();  //call reset

  if (bufblk) {
    // the I2C buffer is full and needs to be processed
    message_t *message = (message_t *)buf;

    DEBUG_PRINTLN("packet received");
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

    String msg = String(message->id) + String(", ") + String(message->counter) + String(", ") + String(message->value1) + String(", ") + String(message->value2) + String(", ") + String(message->value3) + String(", ") + String(message->value4) + String(", ") + String(message->value5) + String(", ") + (message->crc);
    msg.toCharArray(stringbuf, 255);
    Syslog.logger(1, 6, "", "rfm12b_thingspeak:", stringbuf);

    unsigned long check = crc_buf((char *)message, sizeof(message_t) - sizeof(unsigned long));

    if (message->crc == check) {

      // forward the received message
      if (!client.connected()) {
        String postString;
        postString += "&field1=" + String(message->value1) + "&field2=" + String(message->value2) + "&field3=" + String(message->value3) + "&field4=" + String(message->value4) + "&field5=" + String(message->value5) + "&field6=" + String(message->counter);
        lastConnectionTime = millis();
        switch (message->id) {
          case 2:
            updateThingSpeak(postString, APIKeyChannel2);
            break;
          case 3:
            updateThingSpeak(postString, APIKeyChannel3);
            break;
          case 4:
            updateThingSpeak(postString, APIKeyChannel4);
            break;
          case 5:
            updateThingSpeak(postString, APIKeyChannel5);
            break;
          case 6:
            updateThingSpeak(postString, APIKeyChannel6);
            break;
        } // switch
      }
      else {
        DEBUG_PRINTLN(F("not connected"));
      }

    }
    else {
      DEBUG_PRINTLN(F("CRC mismatch"));
    }

    bufblk = 0; // release buffer
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

    if (resetCounter >= 5 ) {
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
  DEBUG_PRINT("receiveEvent ");
  DEBUG_PRINT(howMany);
  DEBUG_PRINT(" : ");

  while (howMany-- > 0) {
    int x = Wire.read(); // receive byte as an integer
    DEBUG_PRINT(x);
    DEBUG_PRINT(" ");

    if (!bufblk)
      buf[bufptr++] = x; // insert into buffer

    if ((bufptr % 32) == 0) {
      bufptr = 0; // point to the first element
      bufblk = 1; // block buffer
    }
  }
  DEBUG_PRINTLN("");

} // receiveEvent

/*******************************************************************************************************************/

static const uint32_t PROGMEM crc_table[16] = {
  0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
  0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
  0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
  0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

unsigned long crc_update(unsigned long crc, byte data) {
  byte tbl_idx;
  tbl_idx = crc ^ (data >> (0 * 4));
  crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
  tbl_idx = crc ^ (data >> (1 * 4));
  crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
  return crc;
}

unsigned long crc_buf(char *b, long l) {
  unsigned long crc = ~0L;
  for (unsigned long i = 0; i < l; i++)
    crc = crc_update(crc, ((char *)b)[i]);
  crc = ~crc;
  return crc;
}

unsigned long crc_string(char *s) {
  unsigned long crc = ~0L;
  while (*s)
    crc = crc_update(crc, *s++);
  crc = ~crc;
  return crc;
}
