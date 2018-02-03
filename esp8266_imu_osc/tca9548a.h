#ifndef __TCA9548A_H_
#define __TCA9548A_H_

#define TCA9548A_ADDRESS    0x70

class tca9548a {
  public:
    ~tca9548a(void);
    tca9548a(void);
    void select(uint8_t);
    void disable();
  private:
    uint8_t _address;

}; // class

#endif

