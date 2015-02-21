#include <OneWire.h>       // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <JeeLib.h>        // https://github.com/jcw/jeelib
#include <Ports.h>         // https://github.com/jcw/jeelib
#include <avr/pgmspace.h>  // http://excamera.com/sphinx/article-crc.html
#include <avr/sleep.h>

//#define BLIP_DEBUG
//#define USE_SLEEPY
#define BLIP_NODE 6   // set this to a unique NODE to disambiguate multiple nodes
#define BLIP_GRP  17  // wireless net group to use for sending blips

#define SEND_MODE     2   // set to 3 if fuses are e=06/h=DE/l=CE, else set to 2
#define RF_CHIPSELECT 10

OneWire ds(9);        // two ds18b20's are connected to this pin 

// this must be defined since we're using the watchdog for low-power waiting
ISR(WDT_vect) { 
  Sleepy::watchdogEvent(); 
}

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
  Serial.print("\n[rfm12b_send_ds18b20 / ");
  Serial.print(__DATE__);
  Serial.print(" / ");
  Serial.print(__TIME__);
  Serial.println("]");
#endif

  pinMode(RF_CHIPSELECT, OUTPUT);
  rf12_set_cs(RF_CHIPSELECT);
  rf12_initialize(BLIP_NODE, RF12_868MHZ, BLIP_GRP);
  rf12_sleep(RF12_SLEEP);

  delay(100); // give it some time to send before falling asleep

  message.id          = BLIP_NODE;
  message.counter     = 0;
  message.value1      = NAN;
  message.value2      = NAN;
  message.value3      = NAN;
  message.value4      = NAN;
  message.value5      = NAN;
}

/*****************************************************************************/

void loop () {

  byte addr1[8] = {
    0x28, 0x50, 0x49, 0x6B, 0x05, 0x00, 0x00, 0x1B          };
  byte addr2[8] = {
    0x28, 0x07, 0x61, 0x98, 0x06, 0x00, 0x00, 0x6B          };

  ds.reset();
  ds.select(addr1);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end

  ds.reset();
  ds.select(addr2);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end

  delay(1000);              // maybe 750ms is enough, maybe not

  message.value1      = 0.001*readVcc(); // this is in mV, we want V
  message.value2      = readTemp(addr1);
  message.value3      = readTemp(addr2);
  message.counter    += 1;
  message.crc         = crc_buf((char *)&message, sizeof(message_t) - sizeof(unsigned long));

  rf12_sleep(RF12_WAKEUP);
  rf12_sendNow(0, &message, sizeof message);
  rf12_sendWait(SEND_MODE);
  rf12_sleep(RF12_SLEEP);

#ifdef BLIP_DEBUG
  // DISPLAY DATA
  Serial.print("DS18B20,\t");
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

  delay(100); // give it some time to send before falling asleep

#ifdef USE_SLEEPY
  // this does not work in combination with the OneWire library
  Sleepy::loseSomeTime(66000);
#else
  delay(66000);
#endif
}


/*****************************************************************************/

float readTemp(byte addr[]) {
  byte data[9];
  float celsius, fahrenheit;

  ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( int i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];

  byte cfg = (data[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
  if      (cfg == 0x00) raw = raw & ~7; //  9 bit resolution, 93.75 ms
  else if (cfg == 0x20) raw = raw & ~3; // 10 bit resolution, 187.5 ms
  else if (cfg == 0x40) raw = raw & ~1; // 11 bit resolution, 375 ms
  //// default is 12 bit resolution, 750 ms conversion time

  celsius    = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;

  return celsius;
}

/*****************************************************************************/

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

  result = 1108848L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
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
  byte tbl_NODEx;
  tbl_NODEx = crc ^ (data >> (0 * 4));
  crc = pgm_read_dword_near(crc_table + (tbl_NODEx & 0x0f)) ^ (crc >> 4);
  tbl_NODEx = crc ^ (data >> (1 * 4));
  crc = pgm_read_dword_near(crc_table + (tbl_NODEx & 0x0f)) ^ (crc >> 4);
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





