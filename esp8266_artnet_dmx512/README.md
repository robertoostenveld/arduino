# Overview

This sketch implements a WiFi module that uses the Art-Net protocol over a wireless connection to control wired stage lighting fixtures/lamps that are DMX512 compatible. It listens to incoming Art-Net packets and forwards a single universe to DMX512. It features over-the-air (OTA) configuration of the wifi network that it connects to, configuration of the universe that is forwarded, and monitoring of the incoming packets and the frame rate.

Upon switching on, the led turns yellow to indicate that setup is done. After that the led turns red to indicate that it is not connected to WiFi. It will try connect as client to the previously used WiFi network; if that succeeds, the led turns green and setup is ready. If that fails, the led remains red and the node creates a wireless access point (AP) with the name Artnet. You can connect with laptop or smartphone to that network to configure the WIFi client settings and provide the password of the network to which it should connect. After that it resets.

Wherever there is activity on the web interface (configuration, monitoring), the led turns blue. During web interface activity, the DMX512 output is silenced. A smooth web interface and smooth DMX signalling don't go together.

See http://robertoostenveld.nl/art-net-to-dmx512-with-esp8266/ for more details and photo's.

# Components
 - NodeMCU, Wemos D1 mini or other ESP-8266 module
 - MAX485 module, e.g.  http://ebay.to/2iuKQlr
 - DC-DC boost/buck converter 5V power supply, e.g. http://ebay.to/2iAfOei
 - L1117 3.3V Low Dropout Linear Regulator (LDO)
 - 3 or 5 pin female XLR connector

# Wiring scheme
 - connect 5V and GND from the power supply to Vcc and GND of the MAX485 module
 - connect 5V and GND from the power supply to the LD1117 LDO
 - connect the 3.3V output and the GND of the LD1117 LDO to the Vcc of the ESP module
 - connect pin DE (data enable) of the MAX485 module to 3.3V (using 3.3V TTL)
 - connect pin RE (receive enable) of the MAX485 module to GND
 - connect pin D4/TX1 of the Wemos mini to the DI (data in) pin of the MAX485 module (using 3.3V TTL)
 - connect pin A to XLR 3
 - connect pin B to XLR 2
 - connect GND   to XLR 1
