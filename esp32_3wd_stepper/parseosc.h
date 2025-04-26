#ifndef _PARSEOSC_H_
#define _PARSEOSC_H_

#include <WiFiUdp.h>
#include <OSCMessage.h>         // https://github.com/CNMAT/OSC
#include <OSCBundle.h>

// these are defined in the main sketch
extern float vx, vy, va;
extern float x, y, a;
extern WiFiUDP Udp;
void startRoute(int);
void pauseRoute();
void resumeRoute();
void stopRoute();
void resetRoute();

// these are defined in the corresponding CPP file
void parseOSC();
void printCallback(OSCMessage &);
void route1Callback(OSCMessage &);
void route2Callback(OSCMessage &);
void route3Callback(OSCMessage &);
void route4Callback(OSCMessage &);
void route5Callback(OSCMessage &);
void route6Callback(OSCMessage &);
void route7Callback(OSCMessage &);
void route8Callback(OSCMessage &);
void pauseCallback(OSCMessage &);
void resumeCallback(OSCMessage &);
void stopCallback(OSCMessage &);
void resetCallback(OSCMessage &);
void xCallback(OSCMessage &);
void yCallback(OSCMessage &);
void aCallback(OSCMessage &);
void dxCallback(OSCMessage &);
void dyCallback(OSCMessage &);
void daCallback(OSCMessage &);
void xyCallback(OSCMessage &);
void accxyzCallback(OSCMessage &);

#endif
