#include <JeeLib.h>        // https://github.com/jcw/jeelib
#include <Ports.h>         // https://github.com/jcw/jeelib
#include <avr/pgmspace.h>  // http://excamera.com/sphinx/article-crc.html
#include <avr/sleep.h>

#define BLIP_DEBUG
#define BLIP_NODE 5   // set this to a unique NODE to disambiguate multiple nodes
#define BLIP_GRP  17  // wireless net group to use for sending blips

#define SEND_MODE     2   // set to 3 if fuses are e=06/h=DE/l=CE, else set to 2
#define RF_CHIPSELECT 10

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
  Serial.print("\n[rfm12b_send_random / ");
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
  boolean stable = false;

  long vcc = readVcc();
  message.value1  = 0.001*readVcc(); // this is in mV, we want V
  message.value2  = (float)(random(2147483647L))/(2147483647L);
  message.value3  = (float)(random(2147483647L))/(2147483647L);
  message.value4  = (float)(random(2147483647L))/(2147483647L);
  message.value5  = (float)(random(2147483647L))/(2147483647L);
  message.counter += 1;
  message.crc     = crc_buf((char *)&message, sizeof(message_t) - sizeof(unsigned long));

  rf12_sleep(RF12_WAKEUP);
  rf12_sendNow(0, &message, sizeof message);
  rf12_sendWait(SEND_MODE);
  rf12_sleep(RF12_SLEEP);

#ifdef BLIP_DEBUG
  // DISPLAY DATA
  Serial.print("RANDOM,\t");
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
  delay(100); // give it some time to send before falling asleep
#endif

  Sleepy::loseSomeTime(65000);
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

PROGMEM const uint32_t crc_table[16] = {
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
