#include "send_break.h"
#include "c_types.h"
#include "eagle_soc.h"
#include "uart_register.h"
#include "Arduino.h"

// More detailled information on the DMX protocol can be found on
// http://www.erwinrol.com/dmx512/
// https://erg.abdn.ac.uk/users/gorry/eg3576/DMX-frame.html

// there are two different implementations, both should work
#define SERIAL_BREAK

/* UART for DMX output */
#define SEROUT_UART 1

/* DMX minimum timings per E1.11 */
#define DMX_BREAK 92
#define DMX_MAB 12

void sendBreak() {
#ifdef SERIAL_BREAK
  // switch to another baud rate, see https://forum.arduino.cc/index.php?topic=382040.0
  Serial1.flush();
  Serial1.begin(90000, SERIAL_8N2);
  while(Serial1.available()) Serial1.read();
  // send the break as a "slow" byte
  Serial1.write(0);
  // switch back to the original baud rate
  Serial1.flush();
  Serial1.begin(250000, SERIAL_8N2);
  while(Serial1.available()) Serial1.read();
#else
  // send break using low-level code
  SET_PERI_REG_MASK(UART_CONF0(SEROUT_UART), UART_TXD_BRK);
  delayMicroseconds(DMX_BREAK);
  CLEAR_PERI_REG_MASK(UART_CONF0(SEROUT_UART), UART_TXD_BRK);
  delayMicroseconds(DMX_MAB);
#endif
}
