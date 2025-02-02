#ifndef _STEPPER_H_
#define _STEPPER_H_

#include <Arduino.h>

class Stepper {
  public:
    Stepper();
    void begin(unsigned int in1, int in2, int in3, int in4);
    void spin(int speed);
    void powerOn();
    void powerOff();

  private:
    unsigned int frequency = 10000000;
    unsigned int halfstep[8] = {0b1000, 0b1100, 0b0100, 0b0110, 0b0010, 0b0011, 0b0001, 0b1001};  // half-step drive
    unsigned int in1, in2, in3, in4;  // the ESP32 pins connected to the ULN2003 driver board
    int step = 0;                     // from 0 to 7, wraps around
    int direction = 1;                // +1 or -1
    bool power = true;                // true or false
    hw_timer_t *timer = NULL;

    static void IRAM_ATTR onTimer(void* arg);   // this calls doStep
    void IRAM_ATTR doStep();                    // this implements the actual stepping
};

#endif // _STEPPER_H_
