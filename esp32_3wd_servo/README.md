# Three-wheel-drive omni-wheel robot platform

This is an Arduino sketch for an ESP32 (LOLIN32 Lite) controlled
three-wheel-drive robot based on TS90D servo motors, similar to
the one documented [here][1] and at various other places online.

By using [omni wheels][2], the robot can move in any direction.

The motors are continuous rotation TS90D servo motors, i.e., these
rotate at a given speed, not towards a given location like normal
servo motors.

The wheels are 38 mm onmi wheels from [Piscinarobots][3]. These
wheels are also available on [AliExpress][4].

To position the motors and wheels at 120 degree angles, I designed
and 3D-printed a circular base. I also designed and 3D-printed the
first set of 38mm omni-wheels following the dimensions of the
Piscinarobots/AliExpress wheels, but found that those were too
rough and not running sideways smoothly enough. Hence I switched
to the commercially fabricated wheels.

The control of the robot is implemented using Open Sound Control
(OSC), using the [TouchOSC][5] app on my iPhone. Actually, it
is not (yet) a robot platform but more like aradio-controlled
car, since there is no autonomous movement.

The reason for me **not continuing** the development of this robot
platform is that the servo motors are too noisy for the intended
purpose, and that they stall when running them at a too low speed.
I switched to using 28BYJ-48 stepper motors; the sketch for that
can be found elsewhere in this repository.

[1]: https://github.com/manav20/3-wheel-omni
[2]: https://en.wikipedia.org/wiki/Omni_wheel
[3]: https://www.piscinarobots.nl/robots-y-kits/38mm%20(1.5%20inch)%20double%20plastic%20omni%20wheel%20(compatible%20met%20servo%20motor%20)%20-%2014184
[4]: https://nl.aliexpress.com/item/32478938051.html
[5]: https://hexler.net
