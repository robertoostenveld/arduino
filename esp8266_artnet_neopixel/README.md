# Overview

This is an Arduino sketch for an ESP8266 module connected to a neopixel LED strip. It uses ArtNet to control features of the light pattern, such as the brightness and the speed. There are a number of modes implemented to make nice patterns, such as a slider that moves from one to the other side.

## Webinterface

The Neopixel and Art-Net settings can be updated on the fly using the webinterface or like this

    curl -X PUT -d '{"universe":1,"offset":0,"pixels":24,"leds":4,"white":0,"brightness":100,"hsv":0,"mode":10,"speed":8,"split":1,"reverse":0}' artnet.local/json

## Operating modes

    mode 0: individual pixel control
    channel 1 = pixel 1 red
    channel 2 = pixel 1 green
    channel 3 = pixel 1 blue
    channel 4 = pixel 1 white
    channel 5 = pixel 2 red
    etc.

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
    channel 9  = intensity (this also allows to black out the colors)
    channel 10 = balance (between color 1 and color2)

    mode 3: single uniform color, blinking between the color and black
    channel 1 = red
    channel 2 = green
    channel 3 = blue
    channel 4 = white
    channel 5 = intensity
    channel 6 = speed (number of flashes per unit of time)
    channel 7 = ramp (whether there is a abrubt or more smooth transition)
    channel 8 = duty cycle (the time ratio between the color and black)

    mode 4: uniform color, blinking between color 1 and color 2
    channel 1  = color 1 red
    channel 2  = color 1 green
    channel 3  = color 1 blue
    channel 4  = color 1 white
    channel 5  = color 2 red
    channel 6  = color 2 green
    channel 7  = color 2 blue
    channel 8  = color 2 white
    channel 9  = intensity
    channel 10 = speed
    channel 11 = ramp
    channel 12 = duty cycle

    mode 5: single color slider, segment that can be moved along the array (between the edges)
    channel 1 = red
    channel 2 = green
    channel 3 = blue
    channel 4 = white
    channel 5 = intensity
    channel 6 = position (from 0-255 or 0-360 degrees, relative to the length of the array)
    channel 7 = width    (from 0-255 or 0-360 degrees, relative to the length of the array)

    mode 6: dual color slider, segment can be moved along the array (between the edges)
    channel 1  = color 1 red
    channel 2  = color 1 green
    channel 3  = color 1 blue
    channel 4  = color 1 white
    channel 5  = color 2 red
    channel 6  = color 2 green
    channel 7  = color 2 blue
    channel 8  = color 2 white
    channel 9  = intensity
    channel 10 = position (from 0-255 or 0-360 degrees, relative to the length of the array)
    channel 11 = width    (from 0-255 or 0-360 degrees, relative to the length of the array)

    mode 7: single color smooth slider, segment can be moved along the array (continuous over the edge)
    channel 1 = red
    channel 2 = green
    channel 3 = blue
    channel 4 = white
    channel 5 = intensity
    channel 6 = position (from 0-255 or 0-360 degrees, relative to the length of the array)
    channel 7 = width    (from 0-255 or 0-360 degrees, relative to the length of the array)
    channel 8 = ramp     (from 0-255 or 0-360 degrees, relative to the length of the array)

    mode 8: dual color smooth slider, segment can be moved along the array (continuous over the edge)
    channel 1  = color 1 red
    channel 2  = color 1 green
    channel 3  = color 1 blue
    channel 4  = color 1 white
    channel 5  = color 2 red
    channel 6  = color 2 green
    channel 7  = color 2 blue
    channel 8  = color 2 white
    channel 9  = intensity
    channel 10 = position (from 0-255 or 0-360 degrees, relative to the length of the array)
    channel 11 = width    (from 0-255 or 0-360 degrees, relative to the length of the array)
    channel 12 = ramp     (from 0-255 or 0-360 degrees, relative to the length of the array)

    mode 9: spinning color wheel
    channel 1 = red
    channel 2 = green
    channel 3 = blue
    channel 4 = white
    channel 5 = intensity
    channel 6 = speed
    channel 7 = width
    channel 8 = ramp

    mode 10: spinning color wheel with color background
    channel 1  = color 1 red
    channel 2  = color 1 green
    channel 3  = color 1 blue
    channel 4  = color 1 white
    channel 5  = color 2 red
    channel 6  = color 2 green
    channel 7  = color 2 blue
    channel 8  = color 2 white
    channel 9  = intensity
    channel 10 = speed
    channel 11 = width
    channel 12 = ramp

    mode 11: rainbow slider
    channel 1 = saturation
    channel 2 = value
    channel 3 = position

    mode 12: rainbow spinner
    channel 1 = saturation
    channel 2 = value
    channel 3 = speed

    mode 13: dual color letter for 8x8 RGBW neopixel array
    channel 1  = color 1 red
    channel 2  = color 1 green
    channel 3  = color 1 blue
    channel 4  = color 1 white
    channel 5  = color 2 red
    channel 6  = color 2 green
    channel 7  = color 2 blue
    channel 8  = color 2 white
    channel 9  = intensity
    channel 10 = ASCII code

## SPIFFS for static files

You should not only write the firmware to the ESP8266 module, but also the static content for the web interface. The html, css and javascript files located in the data directory should be written to the SPIFS filesystem on the ESP8266. See for example http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html and https://www.instructables.com/id/Using-ESP8266-SPIFFS for instructions.

You will get a "file not found" error if the firmware cannot access the data files.

## Arduino ESP8266 filesystem uploader

This Arduino sketch includes a `data` directory with a number of files that should be uploaded to the ESP8266 using the [SPIFFS filesystem uploader](https://github.com/esp8266/arduino-esp8266fs-plugin) tool. At the moment (Feb 2024) the Arduino 2.x IDE does *not* support the SPIFFS filesystem uploader plugin. You have to use the Arduino 1.8.x IDE (recommended), or the command line utilities for uploading the data.
