#define LSB(x) (byte)(x    & 0xff);
#define MSB(x) (byte)(x>>8 & 0xff);

#define pulsePin  A0                // Pulse Sensor purple wire connected to analog pin 0
#define blinkPin  13                // pin to blink led at each beat

// Variables
int TTL = 0;
unsigned long ledTime = 0, flushTime = 0;
unsigned long prevSample = 0;
byte buf[17];

// Volatile Variables, used in the interrupt service routine!
volatile unsigned long sampleCounter = 0;   // used to determine pulse timing
volatile int Signal;                // int that holds raw Analog in 0. This is updated every 2mS
volatile int BPM;                   // int that holds the beats per minute
volatile int IBI = 600;             // int that holds the inter beat interval. Must be seeded!
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean QS = false;        // becomes true when Arduino finds a beat.

void setup() {
  pinMode(pulsePin, INPUT);         // connected to PulseSensor analog output
  pinMode(blinkPin, OUTPUT);        // pin that will blink to your heartbeat!

  Serial.begin(57600);
  unsigned long tic = millis();
  while ((millis() - tic) < 1000 && !(Serial)); // max 1 second

  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS

  // analogReference(EXTERNAL);

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

  // the processing by the interrupt handler is done every 2 ms, i.e. at 500Hz
  // the OpenEEG data format expects sampling to be at 256 Hz, so we write every 4 ms
  if ((sampleCounter > prevSample) && (sampleCounter % 4) == 0) {
    buf[ 3]++;
    buf[ 4] = MSB(Signal*32);
    buf[ 5] = LSB(Signal*32);
    buf[ 6] = MSB(TTL);
    buf[ 7] = LSB(TTL);
    buf[ 8] = MSB(BPM);
    buf[ 9] = LSB(BPM);
    buf[10] = MSB(IBI);
    buf[11] = LSB(IBI);
    buf[12] = MSB(sampleCounter);
    buf[13] = LSB(sampleCounter);
    prevSample = sampleCounter;
    // Write the packet to the serial port
    for (int i = 0; i < 17; i++) {
      Serial.write(buf[i]);
    }
  }

  if ((millis() - flushTime) > 100) {
    // Flush the serial port now and then
    Serial.flush();
    flushTime = millis();
  }

  if ((millis() - ledTime) > 100) {
    // Switch off the LED and the TTL signal
    analogWrite(blinkPin, 0);
    TTL = 0;
  }

  if (QS == true) {
  // Switch on the LED and the TTL signal
  ledTime = millis();
    analogWrite(blinkPin, 255);
    TTL = 512;
    QS = false;
  }

  // All timing critical code is implemented by means of an interrupt
  // Take a short break in the main loop
  delay(2);
}


