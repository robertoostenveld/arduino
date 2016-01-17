#include <stdlib.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <RF12.h>                // https://github.com/jcw/jeelib
#include "secret.h"

#define BLIP_NODE 1  // set this to a unique ID to disambiguate multiple nodes
#define BLIP_GRP  17 // wireless net group to use for sending blips

#define RF_CHIPSELECT 10

#define SOFTWARE_TX 6
#define SOFTWARE_RX 7

#define IP   "184.106.153.149" // thingspeak.com

/*
 * The following are defined in secret.h
 * 
 #define SSID "XXXXX"
 #define PASS "XXXXX"
 #define writeAPIKey1 "XXXXXXXXXXXXXXXX" // Write API Key for a ThingSpeak Channel
 #define writeAPIKey2 "XXXXXXXXXXXXXXXX" // Write API Key for a ThingSpeak Channel
 #define writeAPIKey3 "XXXXXXXXXXXXXXXX" // Write API Key for a ThingSpeak Channel
 */

void(* resetArduino) (void) = 0; //declare reset function at address 0

const unsigned long updateInterval = 30000; // interval in milliseconds between updates, see https://thingspeak.com/docs/channels#rate_limits

// Variable Setup
unsigned long lastConnectionTime = 0; 
boolean lastConnectionStatus = false;
unsigned int lastChannel = 1; // this should start at 1, 2 or 3

SoftwareSerial wifi(SOFTWARE_RX, SOFTWARE_TX);

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

// the receiving rmf12b module itself is number 1, which is why it is not listed here 
message_t message2, message3, message4, message5, message6, message7;
boolean update2 = false, update3 = false, update4 = false, update5 = false, update6 = false, update7 = false;

/*****************************************************************************/

void setup () {
  Serial.begin(57600);
  Serial.print("\n[rfm12b_esp2866 / ");
  Serial.print(__DATE__);
  Serial.print(" / ");
  Serial.print(__TIME__);
  Serial.println("]");

  pinMode(RF_CHIPSELECT, OUTPUT);
  rf12_set_cs(RF_CHIPSELECT);
  rf12_initialize(BLIP_NODE, RF12_868MHZ, BLIP_GRP);

  wifi.begin(9600);

  sendDebug("AT");
  delay(5000);

  if(wifi.find("OK")){
    Serial.println("RECEIVED: OK");
    lastConnectionStatus = connectWiFi();
  }
  else{
    Serial.println("Ooops, no ESP2866 detected ... Check your wiring!");
    resetArduino();
  }
}

/*****************************************************************************/

void loop () {

  // Ensure that the ESP8266 is connected to the wifi network
  if (!lastConnectionStatus) {
    lastConnectionStatus = resetWiFi();
    lastConnectionStatus = connectWiFi();
  }

  // Process incoming messages from the RFM12b 
  if (rf12_recvDone()) {
    if(RF12_WANTS_ACK){
      rf12_sendStart(RF12_ACK_REPLY,0,0);
    }

    if (rf12_crc!=0) {
      Serial.println("Invalid crc");
    }
    else if (rf12_len!=sizeof(message_t)) {
      Serial.println("Invalid len");
    }
    else {
      // volatile byte rf12_hdr  - Contains the header byte of the received packet - with flag bits and node ID of either the sender or the receiver.
      // volatile byte rf12_len  - The number of data bytes in the packet. A value in the range 0 .. 66.
      // volatile byte rf12_data - A pointer to the received data.
      // volatile byte rf12_crc  - CRC of the received packet, zero indicates correct reception. If != 0 then rf12_hdr, rf12_len, and rf12_data should not be relied upon.

      message_t *message = (message_t *)rf12_data;

      unsigned long check = crc_buf((char *)message, sizeof(message_t) - sizeof(unsigned long));

      if (message->crc==check) {

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
      else {
        Serial.print("checksum mismatch ");
        Serial.print(message->crc);
        Serial.print(" != ");
        Serial.println(check);
      }

      // DISPLAY DATA
      Serial.print(message->id);
      Serial.print(",\t");
      Serial.print(message->counter);
      Serial.print(",\t");
      Serial.print(message->value1, 2);
      Serial.print(",\t");
      Serial.print(message->value2, 2);
      Serial.print(",\t");
      Serial.print(message->value3, 2);
      Serial.print(",\t");
      Serial.print(message->value4, 2);
      Serial.print(",\t");
      Serial.print(message->value5, 2);
      Serial.print(",\t");
      Serial.println(message->crc);
    }
  }

  // update ThingSpeak
  if (millis() - lastConnectionTime > updateInterval) {

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


    if (thisChannel) {
      // one of the channels should be updated

      Serial.print(update2);
      Serial.print(update3);
      Serial.print(update4);
      Serial.print(update5);
      Serial.print(update6);
      Serial.println(update7);

      Serial.print("lastChannel = ");
      Serial.println(lastChannel);
      Serial.print("thisChannel = ");
      Serial.println(thisChannel);

      String postString;

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

      if (thisChannel==1)
        updateThingSpeak(postString, writeAPIKey1);
      if (thisChannel==2) 
        updateThingSpeak(postString, writeAPIKey2);
      if (thisChannel==3) 
        updateThingSpeak(postString, writeAPIKey3);

      lastChannel = thisChannel;
      lastConnectionTime = millis();
    }
  }

} // loop

/*******************************************************************************************************************/

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif  

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1155700L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

/*****************************************************************************/

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

/*****************************************************************************/

void sendDebug(String cmd){
  Serial.print("SEND: ");
  Serial.println(cmd);
  Serial.flush();
  wifi.println(cmd);
  wifi.flush();
} // sendDebug

boolean resetWiFi(){
  String cmd;
  cmd = "AT+RST";
  sendDebug(cmd);
  delay(2000);
  cmd = "AT+CWMODE=1";
  sendDebug(cmd);
  delay(2000);
  if(wifi.find("OK")){
    Serial.println("RECEIVED: OK");
    return true;
  }
  else{
    Serial.println("RECEIVED: Error");
    return false;
  }
} // resetWiFi

boolean connectWiFi(){
  String cmd = "AT+CWJAP=\"";
  cmd += SSID;
  cmd += "\",\"";
  cmd += PASS;
  cmd += "\"";
  sendDebug(cmd);
  delay(5000);
  if(wifi.find("OK")){
    Serial.println("WiFi Connect OK");
    return true;
  }
  else{
    Serial.println("WiFi Connect Error");
    return false;
  }
} // connectWiFi

boolean updateThingSpeak(String tsData, String WriteAPIKey) {
  String cmd;
  cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += IP;
  cmd +="\",80";
  sendDebug(cmd);
  delay(2000);
  if(wifi.find("Error")){
    Serial.print("RECEIVED: Error");
    return false;
  }
  cmd = "GET /update?key=";
  cmd += WriteAPIKey;
  cmd += tsData;
  cmd += "\r\n";
  sendDebug(String("AT+CIPSEND=") + String(cmd.length()));
  delay(2000);
  if(wifi.find(">")){
    sendDebug(cmd);
    delay(2000);
    sendDebug("AT+CIPCLOSE");
    Serial.println("DATA: OK");
    return true;
  }
  else {
    sendDebug("AT+CIPCLOSE");
    Serial.println("DATA: Error");
    return false;
  }
} // updateThingSpeak











