#include "send_break.h"
#include "c_types.h"
#include "eagle_soc.h"
#include "uart_register.h"
#include "Arduino.h"

/* UART for DMX output */
#define SEROUT_UART 1

/* DMX minimum timings per E1.11 */
#define DMX_BREAK 92
#define DMX_MAB 12


void sendBreak() {
	SET_PERI_REG_MASK(UART_CONF0(SEROUT_UART), UART_TXD_BRK);
	delayMicroseconds(DMX_BREAK);
	CLEAR_PERI_REG_MASK(UART_CONF0(SEROUT_UART), UART_TXD_BRK);
	delayMicroseconds(DMX_MAB);
}
