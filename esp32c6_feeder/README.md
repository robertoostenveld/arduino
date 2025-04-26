# Pet feeder

This Arduino sketch is for a battery operated pet feeder based on a XIAO ESP32c6. It uses deep 
sleep to conserve power. When it wakes up, it gets a JSON document from a URL that specifies whether 
and how much to feed. It then moves a servo to provide the required number of portions and goes
back to sleep.

The JSON document is dynamically hosted by Node-Red running on a Raspberri Pi, which allows for
reprogramming the feeding schedule.

To prevent the servo motor from jerking to the default 0 angle upon every wake up and also to 
preserve power, it is connected over a bs170 transistor. This alows the ESP to switch the servo
motor power on only when needed.
