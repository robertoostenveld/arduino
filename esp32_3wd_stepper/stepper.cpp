#include "stepper.h"

// This implements the ESP32 control for the 28BYJ-48 motor and the ULN2003 driver board.
//
// It is specifically written to keep up to four stepper motors spinning at a constant speed
// and it uses ESP32 hardware timers for precise timing.
//
// The https://docs.arduino.cc/libraries/stepper/ was not convenient to set and 
// keep the stepper motor runnning at a specific speed.
//
// The https://github.com/bblanchon/ArduinoContinuousStepper library was not 
// precise enough to simultaneously control three stepper motors and to keep 
// my 3-wheel robot platform with omni wheels going straight.
//
// See https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/timer.html

/******************************************************************************/

Stepper::Stepper() {
  // constructor
}

/******************************************************************************/

void Stepper::begin(unsigned int _in1, int _in2, int _in3, int _in4) {
  in1 = _in1;
  in2 = _in2;
  in3 = _in3;
  in4 = _in4;

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  timer = timerBegin(frequency);
  timerAttachInterruptArg(timer, &onTimer, (void*)this);
}

/******************************************************************************/

void Stepper::spin(int speed) {
  if (speed > 0) {
    direction = +1;
    timerAlarm(timer, frequency / (+speed * 2), true, 0); // speed times two since half-steps
  }
  else if (speed < 0) {
    direction = -1;
    timerAlarm(timer, frequency / (-speed * 2), true, 0); // speed times two since half-steps
  }
  else if (speed == 0) {
    direction = 0;
    timerAlarm(timer, 0, false, 0); // execute the alarm once, do not repeat
  }
}

/******************************************************************************/

void Stepper::powerOn() {
  power = true;
}

void Stepper::powerOff() {
  power = false;
}

/******************************************************************************/

void IRAM_ATTR Stepper::onTimer(void* arg) {
  // This is to work around the error that ISO C++ forbids taking the address of
  // an unqualified or parenthesized non-static member function to form a pointer
  // to member function.
  Stepper* pObj = (Stepper*) arg;
  pObj->doStep();
}

/******************************************************************************/

void IRAM_ATTR Stepper::doStep() {
  if (power) {
    digitalWrite(in1, bitRead(halfstep[step], 0));
    digitalWrite(in2, bitRead(halfstep[step], 1));
    digitalWrite(in3, bitRead(halfstep[step], 2));
    digitalWrite(in4, bitRead(halfstep[step], 3));

    // increment or decrement, depending on the direction
    step += direction;

    // wrap between 0 and 7
    if (step < 0)
      step = 7;
    else if (step > 7)
      step = 0;
  }
  else {
    digitalWrite(in1, 0);
    digitalWrite(in2, 0);
    digitalWrite(in3, 0);
    digitalWrite(in4, 0);
  }
}
