# Three-wheel-drive omni-wheel robot platform

This is an Arduino sketch for an ESP32 (LOLIN32 Lite) controlled
three-wheel-drive omni-wheel robot based on 28BYJ-48 stepper motors, similar to the one documented [here][1] and various other places online.

The robot can rotate around its axis and move in any direction using [omni wheels][2]. The wheels are 38 mm omni-wheels that I orderen from [Piscinarobots][3]. These wheels are also available on [AliExpress][4].

To position the motors and wheels at 120 degree angles, I designed
and 3D-printed a circular base. I also designed and 3D-printed the
first set of omni-wheels following the dimensions of the
commercial 38 mm wheels, but found that those were too rough and not running sideways smoothly enough. Hence I switched to the commercially fabricated wheels.

The control of the robot is implemented using Open Sound Control
(OSC), using the [TouchOSC][5] app on my iPhone. The robot can move by itself along a prespecified list of waypoints (and times at which to reach each waypoint). The waypoints are uploaded as CSV file.

## Coordinate system

This sketch aligns with the convention used by [WPILib][6]
and uses an NWU axes convention (North-West-Up as external reference
in the world frame.) In the NWU axes convention, the positive
X axis points ahead, the positive Y axis points left, and the
positive Z axis points up referenced from the floor. When viewed
with each positive axis pointing toward you, counter-clockwise (CCW)
is a positive value and clockwise (CW) is a negative value.

Considering the 3-wheel-drive robot platform, wheel 1 is placed at
the front (the "nose") and wheel 2 and 3 at the left and right back.
The x-axis is pointing fron the center to wheel 1, and the y-axix
pointing to the left. The angle theta is positive when (seen from
the top) the robot turns to the left (CCW) and negative to the right.

## Links

[1]: https://github.com/manav20/3-wheel-omni
[2]: https://en.wikipedia.org/wiki/Omni_wheel
[3]: https://www.piscinarobots.nl/robots-y-kits/38mm%20(1.5%20inch)%20double%20plastic%20omni%20wheel%20(compatible%20met%20servo%20motor%20)%20-%2014184
[4]: https://nl.aliexpress.com/item/32478938051.html
[5]: https://hexler.net
[6]: https://docs.wpilib.org/en/stable/docs/software/basic-programming/coordinate-system.html
