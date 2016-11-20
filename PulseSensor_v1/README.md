# Arduino sketch for PulseSensor, see http://pulsesensor.com

This reads the analog value of the sensor and sends it over the serial port in OpenEEG format. This allows the data to be visualized in [any of these software packages](http://openeeg.sourceforge.net/doc/sw/) or in realtime EGE processing software such as [BCI2000](http://www.bci2000.org) or [FieldTrip](http://www.fieldtriptoolbox.org/development/realtime/modulareeg).

The sample timing is relatively inaccurate. It assumes that the code to read the analog value and to write serial output takes negligible time. At the end of each loop there is a delay of 4ms, resulting in approximately 250 Hz sampling rate.
