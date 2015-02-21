#include <stdlib.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>   // https://learn.adafruit.com/bmp085/using-the-bmp085-api-v2
#include <RF12.h>                // https://github.com/jcw/jeelib

#define BLIP_DEBUG
#define BLIP_NODE 1  // set this to a unique ID to disambiguate multiple nodes
#define BLIP_GRP  17 // wireless net group to use for sending blips

#define RF_CHIPSELECT 10

#define SOFTWARE_TX 6
#define SOFTWARE_RX 7

#define SSID "E900"
#define PASS "XXXXXX"
#define IP   "184.106.153.149" // thingspeak.com

const String writeAPIKey1 = "XXXXXXXXXXXXXXXX"; // Write API Key for a ThingSpeak Channel
const String writeAPIKey2 = "XXXXXXXXXXXXXXXX"; // Write API Key for a ThingSpeak Channel
const String writeAPIKey3 = "XXXXXXXXXXXXXXXX"; // Write API Key for a ThingSpeak Channel
const unsigned long updateInterval = 30000;     // interval in milliseconds between updates, see https://thingspeak.com/docs/channels#rate_limits

boolean lastConnectionStatus;
unsigned long lastConnectionTime = 0; 
unsigned int lastChannel = 0;
String postString1, postString2;

SoftwareSerial wifi(SOFTWARE_RX, SOFTWARE_TX);

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

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

message_t message;

/*****************************************************************************/

void setup () {
#ifdef BLIP_DEBUG
  Serial.begin(57600);
  Serial.print("\n[rfm12b_esp2866 / ");
  Serial.print(__DATE__);
  Serial.print(" / ");
  Serial.print(__TIME__);
  Serial.println("]");
#endif

  wifi.begin(9600);

  pinMode(RF_CHIPSELECT, OUTPUT);
  rf12_set_cs(RF_CHIPSELECT);
  rf12_initialize(BLIP_NODE, RF12_868MHZ, BLIP_GRP);

  /* Initialise the BMP180 sensor */
  if(!bmp.begin()) {
    Serial.println("Ooops, no BMP180 detected ... Check your wiring or I2C ADDR!");
    //while(1);
  }
  else {
    Serial.println("BMP180 detected.");
    displaySensorDetails();
  }

  sendDebug("AT");
  delay(5000);
  if(wifi.find("OK")){
    Serial.println("RECEIVED: OK");
    lastConnectionStatus = connectWiFi();
  }
  else{
    Serial.println("Ooops, no ESP2866 detected ... Check your wiring!");
    //while(1);
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

      memcpy(&message, (void *)rf12_data, sizeof(message_t));

      unsigned long check = crc_buf((char *)&message, sizeof(message_t) - sizeof(unsigned long));
      if (message.crc==check) {

        switch (message.id) {
        case 2:
          // LM35: V, T
          postString1 += "&field1="+String(message.value1)+"&field2="+String(message.value2);
          break;
        case 3:
          // AM2301: V, T, H
          postString1 += "&field3="+String(message.value1)+"&field4="+String(message.value2)+"&field5="+String(message.value3);
          break;
        case 4:
          // BMP085: V, T, P
          // FIXME field 7 and 8 are swapped in the ThingSpeak channel
          postString1 += "&field6="+String(message.value1)+"&field8="+String(message.value2)+"&field7="+String(message.value3);
          break;
        case 5:
          // 2x CNY50: V, KWh, M3
          postString2 += "&field1="+String(message.value1)+"&field2="+String(message.value2)+"&field3="+String(message.value3);
          break;
        case 6:
          // 2x DS18B20: V, T, T
          postString2 += "&field4="+String(message.value1)+"&field5="+String(message.value2)+"&field6="+String(message.value3);
          break;
        } // switch
      }
      else {
        Serial.print("checksum mismatch ");
        Serial.print(message.crc);
        Serial.print(" != ");
        Serial.println(check);
      }

      // DISPLAY DATA
      Serial.print(message.id);
      Serial.print(",\t");
      Serial.print(message.counter);
      Serial.print(",\t");
      Serial.print(message.value1, 2);
      Serial.print(",\t");
      Serial.print(message.value2, 2);
      Serial.print(",\t");
      Serial.print(message.value3, 2);
      Serial.print(",\t");
      Serial.print(message.value4, 2);
      Serial.print(",\t");
      Serial.print(message.value5, 2);
      Serial.print(",\t");
      Serial.println(message.crc);
    }
  }

  // Send the most recent data to ThingSpeak
  if ((millis() - lastConnectionTime) > updateInterval) {

    // we can only connect once every 15 seconds 
    // only a single channel can be updated per connection

    if (lastChannel==2 && postString1.length()>0) {
      Serial.println("Updating channel 1");
      lastConnectionStatus = updateThingSpeak(postString1, writeAPIKey1);
      postString1 = "";
      lastChannel = 1;    
    }
    else if (lastChannel==1 && postString2.length()>0) {
      Serial.println("Updating channel 2");
      lastConnectionStatus = updateThingSpeak(postString2, writeAPIKey2);
      postString2 = "";
      lastChannel = 2;
    }
    else if (postString1.length()>0) {
      Serial.println("Updating channel 1");
      lastConnectionStatus = updateThingSpeak(postString1, writeAPIKey1);
      postString1 = "";
      lastChannel = 1;
    }
    else if (postString2.length()>0) {
      Serial.println("Updating channel 2");
      lastConnectionStatus = updateThingSpeak(postString2, writeAPIKey2);
      postString2 = "";
      lastChannel = 2;
    }
    lastConnectionTime = millis();
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

void displaySensorDetails(void)
{
  sensor_t sensor;
  bmp.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); 
  Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); 
  Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); 
  Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); 
  Serial.print(sensor.max_value); 
  Serial.println(" hPa");
  Serial.print  ("Min Value:    "); 
  Serial.print(sensor.min_value); 
  Serial.println(" hPa");
  Serial.print  ("Resolution:   "); 
  Serial.print(sensor.resolution); 
  Serial.println(" hPa");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

/*****************************************************************************/

void getSensorMeasurement(void) 
{
  boolean stable = false;
  float pressure, temperature;

  bmp.getTemperature(&temperature);
  bmp.getPressure(&pressure);

  message.value1      = 0.001*readVcc(); // this is in mV, we want V
  message.value2      = temperature;     // this is in Celcius
  message.value3      = 0.01*pressure;   // this is in Pa, we want hPa, i.e. mbar
  message.counter    += 1;
  message.crc         = crc_buf((char *)&message, sizeof(message_t) - sizeof(unsigned long));

  // DISPLAY DATA
#ifdef BLIP_DEBUG
  Serial.print("BMP180,\t");
  Serial.print(message.id);
  Serial.print(",\t");
  Serial.print(message.counter);
  Serial.print(",\t");
  Serial.print(message.value1, 2);
  Serial.print(",\t");
  Serial.print(message.value2, 2);
  Serial.print(",\t");
  Serial.print(message.value3, 2);
  Serial.print(",\t");
  Serial.print(message.value4, 2);
  Serial.print(",\t");
  Serial.print(message.value5, 2);
  Serial.print(",\t");
  Serial.println(message.crc);
#endif
}

/*****************************************************************************/

void sendDebug(String cmd){
  Serial.print("SEND: ");
  Serial.println(cmd);
  Serial.flush();
  wifi.println(cmd);
} // sendDebug

boolean resetWiFi(){
  String cmd = "AT+RST";
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
  String cmd = "AT+CWMODE=1";
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
  cmd  = "AT+CWJAP=\"";
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
  cmd = String("AT+CIPSTART=\"TCP\",\"") + String(IP) + String("\",80");
  Serial.println(cmd);
  delay(2000);
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
  wifi.print("AT+CIPSEND=");
  wifi.println(cmd.length());
  if(wifi.find(">")){
    Serial.print(">");
    Serial.println(cmd);
    wifi.print(cmd);
    Serial.println("DATA: OK");
    delay(2000);
    sendDebug("AT+CIPCLOSE");
    return true;
  }
  else {
    Serial.println("DATA: Error");
    delay(2000);
    sendDebug("AT+CIPCLOSE");
    return false;
  }
} // updateThingSpeak






