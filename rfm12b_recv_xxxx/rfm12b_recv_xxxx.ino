#include <RF12.h>          // https://github.com/jcw/jeelib
#include <Wire.h>

#define BLIP_DEBUG
#define BLIP_NODE 1  // set this to a unique ID to disambiguate multiple nodes
#define BLIP_GRP  17 // wireless net group to use for sending blips

#define RF_CHIPSELECT   10

#define getbyte1(x) (x>> 0 & 0xff)
#define getbyte2(x) (x>> 8 & 0xff)
#define getbyte3(x) (x>>16 & 0xff)
#define getbyte4(x) (x>>24 & 0xff)

typedef struct payload_t {
  unsigned long id;
  unsigned long counter;
  float value1;
  float value2;
  float value3;
  float value4;
  float value5;
  unsigned long crc;
};

payload_t payload;

/*****************************************************************************/

void setup () {
#ifdef BLIP_DEBUG
  Serial.begin(57600);
  Serial.print("\n[rfm12b_recv_xxxx / ");
  Serial.print(__DATE__);
  Serial.print(" / ");
  Serial.print(__TIME__);
  Serial.println("]");
#endif

  Wire.begin(); // Start I2C Bus as Master

  pinMode(RF_CHIPSELECT, OUTPUT);
  rf12_set_cs(RF_CHIPSELECT);
  rf12_initialize(BLIP_NODE, RF12_868MHZ, BLIP_GRP);
}

/*****************************************************************************/

void loop () {

  // volatile byte rf12_hdr  - Contains the header byte of the received packet - with flag bits and node ID of either the sender or the receiver.
  // volatile byte rf12_len  - The number of data bytes in the packet. A value in the range 0 .. 66.
  // volatile byte rf12_data - A pointer to the received data.
  // volatile byte rf12_crc  - CRC of the received packet, zero indicates correct reception. If != 0 then rf12_hdr, rf12_len, and rf12_data should not be relied upon.

  if (rf12_recvDone()) {
    if(RF12_WANTS_ACK){
      rf12_sendStart(RF12_ACK_REPLY,0,0);
    }

    if (rf12_crc!=0) {
      Serial.println("Invalid crc");
    }
    else if (rf12_len!=sizeof payload) {
      Serial.println("Invalid len");
    }
    else {
      memcpy(&payload, (void *)rf12_data, sizeof payload);

      unsigned long check = crc_buf((char *)&payload, sizeof(payload_t) - sizeof(unsigned long));
      if (payload.crc==check) {

        write_i2c((byte *)&payload.id,      sizeof(unsigned long));
        write_i2c((byte *)&payload.counter, sizeof(unsigned long));
        write_i2c((byte *)&payload.value1,  sizeof(float));
        write_i2c((byte *)&payload.value2,  sizeof(float));
        write_i2c((byte *)&payload.value3,  sizeof(float));
        write_i2c((byte *)&payload.value4,  sizeof(float));
        write_i2c((byte *)&payload.value5,  sizeof(float));
        write_i2c((byte *)&payload.crc,     sizeof(unsigned long));
      }
      else {
        Serial.print("checksum mismatch ");
        Serial.print(payload.crc);
        Serial.print(" != ");
        Serial.println(check);
      }

      // DISPLAY DATA
      Serial.print(payload.id);
      Serial.print(",\t");
      Serial.print(payload.counter);
      Serial.print(",\t");
      Serial.print(payload.value1, 2);
      Serial.print(",\t");
      Serial.print(payload.value2, 2);
      Serial.print(",\t");
      Serial.print(payload.value3, 2);
      Serial.print(",\t");
      Serial.print(payload.value4, 2);
      Serial.print(",\t");
      Serial.print(payload.value5, 2);
      Serial.print(",\t");
      Serial.println(payload.crc);
    }
  }
} // loop

/*****************************************************************************/

void transmitfloat(float x) {
  byte *b = (byte *)&x; 
  Wire.beginTransmission(9); // transmit to device #9
  Wire.write(b[0]);
  Wire.write(b[1]);
  Wire.write(b[2]);
  Wire.write(b[3]);
  Wire.endTransmission();    // stop transmitting
  delay(5); // otherwise transmission hangs
}

void transmitlong(long x) {
  byte *b = (byte *)&x; 
  Wire.beginTransmission(9); // transmit to device #9
  Wire.write(b[0]);
  Wire.write(b[1]);
  Wire.write(b[2]);
  Wire.write(b[3]);
  Wire.endTransmission();    // stop transmitting
  delay(5); // otherwise transmission hangs
}

void write_i2c(byte *b, int n) {
  Wire.beginTransmission(9); // transmit to device #9
  Wire.write(b[0]);
  Wire.write(b[1]);
  Wire.write(b[2]);
  Wire.write(b[3]);
  Wire.endTransmission();    // stop transmitting
  delay(5); // otherwise the next transmission hangs
} 

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








