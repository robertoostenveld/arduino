/*
  This sketch is for an ESP8266 connected to an ARCTIC BioniX F140
  140 mm diameter fan with PWM control.

  See https://www.arctic.de/media/55/b2/e4/1690270670/Spec_Sheet_BioniX_F140_EN.pdf
  
  The sketch controls the speed of the fan and uses an interrupt 
  to measure the speed. Output is provided on the serial interface.

*/

#include <Arduino.h>

const int controlPin = D1;
const int sensorPin = D2;

void ICACHE_RAM_ATTR callback();

const char* version = __DATE__ " / " __TIME__;

volatile unsigned int counter = 0;

void setup() {
  Serial.begin(115200);
  Serial.print("\n[esp8266_fan_control / ");
  Serial.print(version);
  Serial.println("]");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  analogWriteFreq(40000);
  pinMode(controlPin, OUTPUT);
  analogWrite(controlPin, 255);  // this is where the speed is controlled

  pinMode(sensorPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(sensorPin), callback, RISING);
}

void loop() {
  unsigned int rps, rpm;

  noInterrupts();
  rps = counter;
  counter = 0;
  interrupts();

  // convert from counts per second to rotations per minute
  rpm = rps * 60 / 2;

  Serial.print("Counts: ");
  Serial.print(rps, 1);
  Serial.print(" RPM: ");
  Serial.println(rpm);

  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

  delay(1000);  // run once per second
}

void callback() {
  counter++;
}
