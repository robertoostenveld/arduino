# Three-wheel-drive omni-wheel robot platform

This is an Arduino sketch for an ESP32 (LOLIN32 Lite) controlled three-wheel-drive omni-wheel robot based on 28BYJ-48 stepper motors, similar to the one documented [here][1] and various other places online.

The robot can rotate in-place and move in any direction using [omni wheels][2]. I ordered 38 mm diameter wheels from [Piscinarobots][3], but they are also available on [AliExpress][4].

To position the motors and wheels at 120 degree angles, I designed and 3D-printed a circular base. I also designed and 3D-printed the first set of omni-wheels following the dimensions of the commercial 38 mm wheels, but found that those were too rough and not running sideways smoothly enough. Hence I switched to the commercially fabricated wheels.

The control of the robot is implemented using Open Sound Control (OSC), using the [TouchOSC][5] app on my iPhone. The robot can move by itself along a prespecified list of waypoints. The waypoints are uploaded as tabular CSV file, which also contains a column with the time at which each waypoint is to be reached.

## Coordinate system

This sketch aligns with the convention used by [WPILib][6] and uses an NWU axes convention (North-West-Up as external reference in the world frame.) In the NWU axes convention, the positive X axis points ahead, the positive Y axis points left, and the positive Z axis points up referenced from the floor. When viewed with each positive axis pointing toward you, counter-clockwise (CCW) is a positive value and clockwise (CW) is a negative value.

Considering the 3-wheel-drive robot platform, wheel 1 is placed at the front (the "nose") and wheel 2 and 3 at the left and right back. The x-axis is pointing fron the center to wheel 1, and the y-axix pointing to the left. The angle theta is positive when (seen from the top) the robot turns to the left (CCW) and negative to the right.

## Settings

In the settings menu you can change the following parameters.

### Repeat

This can be 0 (false) or 1 (true) and specifies whether a route should be repeated when the end of the route is reached.

### Absolute

This can be 0 (false) for relative coordinates) or 1 (true) for absolute coordinates.

When configured as relative, you specify the waypoints and movements _relative_ to the robot's position at the moment each route starts. The starting position (and angle) is set to zero and every new route is executed without taking any previously executed routes into consideration.

When configured as absolute, you specify all waypoints relative to the robot's initial position and orientation in the room. WHen starting a new route, the latest known position (and orientation) is taken as the starting point.

### Warp

This is a floating point number by which the time and distances are scaled. By default the warp is 1.00.

If you specify the warp as 2.00, all positions along the route will be multiplied by a factor of 2x and the time between waypoints will be also be multiplied by a factor of 2x. Consequently, the speed for each segment will remain the same, and the overall route will be 2x larger and will take 2x longer.

If you specify the warp as 0.5, the overall route will be 2x shorter and faster

### Serial

This can be 0 (false) or 1 (true) and specifies how much information will be printed on the serial console when the robot is connected to a computer. This is only for debugging.

## Links

[1]: https://github.com/manav20/3-wheel-omni
[2]: https://en.wikipedia.org/wiki/Omni_wheel
[3]: https://www.piscinarobots.nl/robots-y-kits/38mm%20(1.5%20inch)%20double%20plastic%20omni%20wheel%20(compatible%20met%20servo%20motor%20)%20-%2014184
[4]: https://nl.aliexpress.com/item/32478938051.html
[5]: https://hexler.net
[6]: https://docs.wpilib.org/en/stable/docs/software/basic-programming/coordinate-system.html
