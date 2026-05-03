
# Arduino based MEG phantom current driver

This is the firmware for a current driver that supports up to 24 channels, based on three DAC7578 digital-to-analog converters. It is designed to be used with [Adafruit DAC7578 breakout-boards](https://www.adafruit.com/product/6223).

The code will automatically detect whether 1, 2, or three DAC7578 boards are available. Frequencies for the output channels start at a certain offset that you can specify in the code, and subsequent channels have linearly increasing frequencies. Note that to avoid signal analysis problems due to the harmonics of the non-perfect sine waves, the highest frequency should be lower than twice the lowest frequency.

This firmware is compatible with various microcontroller boards, some of which are detailed below. Two factors influence the smoothness of the sine wave:

1. the data transfer rate from the MCU to the DACs
2. the computation of the sine wave values

To improve 1, you want to use the fastest I2C transfer rate supported by your microcontroller. The DAC7578 itself supports up to 3.4 MHz, but different development boards have more limiting rates (see below). The I2C speed can be changed in the code by modifying the argument to the `Wire.setClock` function in the setup.

To improve 2, the code implements a sine wave lookup table as alternative for the `sin` function from the floating-point Math library. There are three functions, with increasing accuracy but also computational costs. The accuracy is furthermore determined by the size of the lookup table which cannot be too large, since otherwise it won't fit into memory. In `lookup.h` you can change the size of the lookup table. A value of 256 works for the Uno R3 and 512 works for the Leonardo which has slightly more memory.

## Arduino Uno R3

The Arduino Uno R3 comes with a USB-B interface for the connection to a computer that can also supply power, or the 2.1 mm barrel plug connector can be used to power the board. The Arduino Uno R3 uses a ATmega328P microcontroller and typically operates its I2C (Wire library) bus at a default speed of 100 kHz (Standard Mode) or 400 kHz (Fast Mode).

Using the firmware version from 3 May 2026, a 256 value lookup table and the `lookup_nearest` implementation, I was able to achieve a stable update frequency of 200 Hz for 24 channels. The Uno R3 does not have enough memory for a 512 value lookup table.

## Arduino Leonardo

The Arduino Leonardo comes with a micro-USB interface for the connection to a computer and to power the board. The Arduino Leonardo uses a ATmega32u4 microcontroller, defaults to an I2C clock speed of 100 kHz and supports a maximum clock speed of 400 kHz.

Using the firmware version from 3 May 2026, a 512 value lookup table and the `lookup_nearest` implementation, I was able to achieve a stable update frequency of 200 Hz for 24 channels.

## Arduino Uno R4

The Arduino Uno R4 comes with a USB-C interface for the connection to a computer that can also supply power, or the 2.1 mm barrel plug connector can be used to power the board. The Arduino Uno R4 uses a Renesas RA4M1 microcontroller, which supports I2C speeds up to 400 kbit/s (Fast Mode) in standard operation and up to 1 Mbit/s (Fast Mode Plus) using the RIIC interface. This should allow faster data transfer from the MCU to the DAC7578 boards, and hence a smoother sine wave.

## Adafruit Feather RP2040

This is a board with the feather form factor, a USB-C interface, and the RP2040 microcontroller. Contrary to the 5V Arduino boards, this is a 3.3V board. The RP2040 can achieve 3.4 MHz I2C speeds, allowing for the fastest transfer of data to the DAC7578 boards.
