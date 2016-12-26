The Neopixel and Art-Net settings can be updated like this
curl -X PUT -d '{"length":24,"leds":4,"universe":1,"offset":0,"mode":3}' 192.168.1.13/settings

Updating the firmware on the Wemos D1 mini board using teh Arduino IDE requires that you install the drivers from https://www.wemos.cc/downloads.

  mode 0: individual pixel control
  channel 1 = pixel 1 red
  channel 2 = pixel 1 green
  channel 3 = pixel 1 blue
  channel 4 = pixel 1 white
  channel 5 = pixel 2 red

  mode 1: single uniform color
  channel 1 = red
  channel 2 = green
  channel 3 = blue
  channel 4 = white
  channel 5 = intensity (this allows scaling a preset RGBW color with a single channel)

  mode 2: two color mixing
  channel 1  = color 1 red
  channel 2  = color 1 green
  channel 3  = color 1 blue
  channel 4  = color 1 white
  channel 5  = color 2 red
  channel 6  = color 2 green
  channel 7  = color 2 blue
  channel 8  = color 2 white
  channel 9  = balance (between color 1 and color2)
  channel 10 = intensity (this also allows to black out the colors)

