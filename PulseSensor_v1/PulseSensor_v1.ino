// PulseSensor application that streams continuous data at 500Hz using the OpenEEG data format
// over the serial port or a BlueTooth connection

#define LSB(x) (byte)(x    & 0xff);
#define MSB(x) (byte)(x>>8 & 0xff);

#define hasUSB
// #define hasBT

unsigned int value = 0, sample = 0;
unsigned long tsample = 4; // should be 4 to approximate 256 Hz according to protocol

byte buf[17];

void setup() {

  pinMode(A0, INPUT);  // connected to PulseSensor analog output

#ifdef hasUSB
  Serial.begin(57600);  // connected to USB
  unsigned long tic = millis();
  while ((millis() - tic) < 1000 && !(Serial)); // max 1 second
#endif

#ifdef hasBT
  Serial1.begin(57600); // connected to BlueSMiRF
  unsigned long tic = millis();
  while ((millis() - tic) < 1000 && !(Serial1)); // max 1 second
#endif

  // the OpenEEG protocol sends data in 17 byte packets
  buf[ 0] = 0xA5;
  buf[ 1] = 0x5A;
  buf[ 2] = 2; // version
  buf[ 3] = 0; // count
  buf[ 4] = 0; // channel 1
  buf[ 5] = 0;
  buf[ 6] = 0; // channel 2
  buf[ 7] = 0;
  buf[ 8] = 0; // channel 3
  buf[ 9] = 0;
  buf[10] = 0; // channel 4
  buf[11] = 0;
  buf[12] = 0; // channel 5
  buf[13] = 0;
  buf[14] = 0; // channel 6
  buf[15] = 0;
  buf[16] = 0; // switches
}

void loop() {

  // read the analog value
  value = analogRead(A0);

  // scale to appropriate values
  value *= 32;

  // increment the sample number
  sample++;

  // update the data packet
  buf[ 3]++;
  buf[ 4] = MSB(value);  // channel 1, MSB
  buf[ 5] = LSB(value);  // channel 1, LSB
  buf[ 6] = MSB(sample); // channel 2, MSB
  buf[ 7] = LSB(sample); // channel 2, LSB

#ifdef hasUSB
  Serial.flush();
  for (int i = 0; i < 17; i++) {
    Serial.write(buf[i]);
  }
#endif

#ifdef hasBT
  Serial1.flush();
  for (int i = 0; i < 17; i++) {
    Serial1.write(buf[i]);
  }
#endif

  delay(tsample);

}  // loop
