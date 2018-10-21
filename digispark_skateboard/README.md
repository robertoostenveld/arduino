# Digispark Skateboard

This is an Arduino sketch to implement a strip of WS2812b Neopixels
under a skateboard. It features multiple blinking and moving patterns
of light, which can be selected using three latching pushbuttons,
allowing for in total 8 modes.

It is implemneted using a
[Digispark](https://www.kickstarter.com/projects/digistump/digispark-the-tiny-arduino-enabled-usb-dev-board)
USB development board and two 50 cm long LED strips, each with 30
LEDS. The LED strips are directly powered from a 3.7-4.2 V 18650
LiPo battery, the Digispark is powered using a small DC-DC transformer
module.

## Light modes

One button is used to switch the power on/off, the other three
buttons allow selecting between up to 8 different light modes. The
implementation of the light modes is borrowed from one of my other
projects: which are [ESP8266 powered ArtNet neopixel
modules](https://robertoostenveld.nl/esp-8266-art-net-neopixel-module/).

* static single uniform color
* uniform blinking between two colors
* rotating color wheel with another color background
* rotating HSV rainbow

## Components

* Digispark rev4
* 18650 LiPo battery with builit-in protection
* battery holder
* USB DC-DC transformer with 5V output
* 4 latching switches, 1x for power, 3x for mode selection
* 3 pull-up resistors, few kOhm
* capacitor, 1000uF
* 3-D printed enclosure

