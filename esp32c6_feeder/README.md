# Pet feeder

This Arduino sketch is for an ESP32-based automated pet feeder system. It uses deep sleep to 
conserve power. When it wakes up, it checks whether to update the clock using NTP and/or whether
it is feeding time. For the feeding it moves a servo motor. After performing its tasks, the ESP32 
goes back to deep sleep.
