/*
 * BITSI - Bits to Serial Interface
 *
 * This is an Arduino Uno based serial/usb interface that can be used to monitor responses from
 * button boxes and to send TTL-level stimulus triggers to the parallel input port of EEG or MEG 
 * acquisition systems.
 * 
 * This code originates from https://code.google.com/archive/p/dccn-lab/ where it was released under the GNU GPL v2
 *
 * Copyright (C) 2009-2010 Erik van der Boogert & Bram Daams (for the original implementation)
 * Copyright (C) 2025 Robert Oostenveld (for this cleaned up version)  
 *
 */

// Constants
const unsigned long TIMEOUT_PULSE_LENGTH = 15;  // Pulse length in ms
const unsigned long TIMEOUT_DEBOUNCE = 8;       // Debounce interval in ms
const int PIN_BUILTIN_LED = 13;

// Input pins configuration
const int INPUT_PINS[8] = {10, 11, 12, 14, 15, 16, 17, 18};

// Variables
byte serialInChar;                  // To receive desired Parallel_Out value
unsigned long timeNow = 0;          // Current time reference
unsigned long timeOnsetOut = 0;     // When Parallel_Out was set
bool isParallelOutActive = false;   // Whether output pulse is active

// Input state tracking
bool buttonState[8] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
bool prevInputState[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};

// Debounce tracking
unsigned long debounceStartTime[8] = {0, 0, 0, 0, 0, 0, 0, 0};
bool isDebouncing[8] = {false, false, false, false, false, false, false, false};

void setup() {
  // Initialize serial port
  Serial.begin(115200);

  // Configure ports
  setupInputPort();
  setupOutputPort();
  writeOutputPort(0);

  // Configure built-in LED
  pinMode(PIN_BUILTIN_LED, OUTPUT);
  digitalWrite(PIN_BUILTIN_LED, LOW);

  // Startup LED sequence
  for (byte i = 0; i < 8; i++) {
    writeOutputPort((B00000001 << (i + 1)) - 1);
    digitalWrite(PIN_BUILTIN_LED, !digitalRead(PIN_BUILTIN_LED));
    delay(100);
  }
  writeOutputPort(0);
  delay(100);

  // Show ready message
  Serial.println("BITSI mode, Ready!");
}

void loop() {
  // Get current time reference
  timeNow = millis();

  // Process serial input
  processSerialInput();

  // Process button pins
  processButtonPins();

  // Handle debounce timeouts
  handleDebounceTimeouts();
}

void processSerialInput() {
  if (Serial.available()) {
    serialInChar = Serial.read();
    writeOutputPort(serialInChar);
    timeOnsetOut = timeNow;
    isParallelOutActive = true;
  } 
  else if (isParallelOutActive && ((timeNow - timeOnsetOut) > TIMEOUT_PULSE_LENGTH)) {
    writeOutputPort(0);
    isParallelOutActive = false;
  }
}

void processButtonPins() {
  for (byte i = 0; i < 8; i++) {
    buttonState[i] = digitalRead(INPUT_PINS[i]);
    
    if ((prevInputState[i] != buttonState[i]) && !isDebouncing[i]) {
      // Send corresponding character (A-H for high, a-h for low)
      Serial.print(char(buttonState[i] == HIGH ? (65 + i) : (97 + i)));
      
      // Start debouncing
      debounceStartTime[i] = millis();
      isDebouncing[i] = true;
      prevInputState[i] = buttonState[i];
    }
  }
}

void handleDebounceTimeouts() {
  for (byte i = 0; i < 8; i++) {
    if (isDebouncing[i] && ((timeNow - debounceStartTime[i]) > TIMEOUT_DEBOUNCE)) {
      isDebouncing[i] = false;
    }
  }
}

void setupOutputPort() {
  for (int pin = 2; pin <= 9; pin++) {
    pinMode(pin, OUTPUT);
  }
}

void setupInputPort() {
  for (int i = 0; i < 8; i++) {
    pinMode(INPUT_PINS[i], INPUT);
    digitalWrite(INPUT_PINS[i], HIGH);  // Enable pull-up resistors
  }
}

byte readInputPort() {
  byte portB = PINB;
  byte portC = PINC;
  return ((portB & B00011100) >> 2) | ((portC & B00011111) << 3);
}

byte readOutputPort() {
  byte portC = PINC;
  byte portD = PIND;
  return ((portD & B11111100) >> 2) | ((portC & B00000011) << 6);
}

void writeOutputPort(byte code) {
  // Calculate new port states
  byte portB = (code & B11000000) >> 6 | (PINB & B11111100);
  byte portD = (code & B00111111) << 2;

  // Update ports
  PORTB = portB;
  PORTD = portD;
}