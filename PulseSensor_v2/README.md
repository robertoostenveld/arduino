Arduino sketch for PulseSensor, see http://pulsesensor.com

This code is derived from the [original code](https://github.com/WorldFamousElectronics/PulseSensor_Amped_Arduino). It uses the same interrupt driven sampling and processing of the data, but the more fancy features have been removed (ASCII art serial output, fading led). It outputs the data at 250Hz over the serial connection in OpenEEG format with the following channels.

1. contains the continuously sampled value
2. contains a TTL-level representation of heart beat events
3. contains the beats per minute
4. contains the inter beat interval in milliseconds
5. contains the sample time in milliseconds
