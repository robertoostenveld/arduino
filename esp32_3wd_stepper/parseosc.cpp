#include "parseosc.h"

/********************************************************************************/

void parseOSC() {
  int size = Udp.parsePacket();

  if (size > 0) {
    OSCMessage msg;
    OSCBundle bundle;
    OSCErrorCode error;

    char c = Udp.peek();
    if (c == '#') {
      // it is a bundle, these are sent by TouchDesigner
      while (size--) {
        bundle.fill(Udp.read());
      }
      if (!bundle.hasError()) {
        bundle.dispatch("/*", printCallback) || bundle.dispatch("/*/*", printCallback) || bundle.dispatch("/*/*/*", printCallback);
        bundle.dispatch("/route/1",     route1Callback);
        bundle.dispatch("/route/2",     route2Callback);
        bundle.dispatch("/route/3",     route3Callback);
        bundle.dispatch("/route/4",     route4Callback);
        bundle.dispatch("/route/5",     route5Callback);
        bundle.dispatch("/route/6",     route6Callback);
        bundle.dispatch("/route/7",     route7Callback);
        bundle.dispatch("/route/8",     route8Callback);
        bundle.dispatch("/pause",       pauseCallback);
        bundle.dispatch("/resume",      resumeCallback);
        bundle.dispatch("/stop",        stopCallback);
        bundle.dispatch("/reset",       resetCallback);
        bundle.dispatch("/adjust/x",    dxCallback);
        bundle.dispatch("/adjust/y",    dyCallback);
        bundle.dispatch("/adjust/a",    daCallback);
        bundle.dispatch("/manual/x",    xCallback);
        bundle.dispatch("/manual/y",    yCallback);
        bundle.dispatch("/manual/a",    aCallback);
        bundle.dispatch("/manual/xy/1", xyCallback);      // this is for the 2D panel in touchOSC
        bundle.dispatch("/accxyz",      accxyzCallback);  // this is for the accelerometer in touchOSC
      } else {
        error = bundle.getError();
        Serial.print("error: ");
        Serial.println(error);
      }
    } else if (c == '/') {
      // it is a message, these are sent by touchOSC
      while (size--) {
        msg.fill(Udp.read());
      }
      if (!msg.hasError()) {
        msg.dispatch("/*", printCallback) || msg.dispatch("/*/*", printCallback) || msg.dispatch("/*/*/*", printCallback);
        msg.dispatch("/route/1",     route1Callback);
        msg.dispatch("/route/2",     route2Callback);
        msg.dispatch("/route/3",     route3Callback);
        msg.dispatch("/route/4",     route4Callback);
        msg.dispatch("/route/5",     route5Callback);
        msg.dispatch("/route/6",     route6Callback);
        msg.dispatch("/route/7",     route7Callback);
        msg.dispatch("/route/8",     route8Callback);
        msg.dispatch("/pause",       pauseCallback);
        msg.dispatch("/resume",      resumeCallback);
        msg.dispatch("/stop",        stopCallback);
        msg.dispatch("/reset",       resetCallback);
        msg.dispatch("/adjust/x",    dxCallback);
        msg.dispatch("/adjust/y",    dyCallback);
        msg.dispatch("/adjust/a",    daCallback);
        msg.dispatch("/manual/x",    xCallback);
        msg.dispatch("/manual/y",    yCallback);
        msg.dispatch("/manual/a",    aCallback);
        msg.dispatch("/manual/xy/1", xyCallback);      // this is for the 2D panel in touchOSC
        msg.dispatch("/accxyz",      accxyzCallback);  // this is for the accelerometer in touchOSC
      } else {
        error = msg.getError();
        Serial.print("error: ");
        Serial.println(error);
      }
    }
  }  // if size
}  // parseOSC

/********************************************************************************/

void printCallback(OSCMessage &msg) {
  Serial.print(msg.getAddress());
  Serial.print(" : ");

  for (int i = 0; i < msg.size(); i++) {
    if (msg.isInt(i)) {
      Serial.print(msg.getInt(i));
    } else if (msg.isFloat(i)) {
      Serial.print(msg.getFloat(i));
    } else if (msg.isDouble(i)) {
      Serial.print(msg.getDouble(i));
    } else if (msg.isBoolean(i)) {
      Serial.print(msg.getBoolean(i));
    } else if (msg.isString(i)) {
      char buffer[256];
      msg.getString(i, buffer);
      Serial.print(buffer);
    } else {
      Serial.print("?");
    }

    if (i < (msg.size() - 1)) {
      Serial.print(", ");  // there are more to come
    }
  }
  Serial.println();
}

/********************************************************************************/

void route1Callback(OSCMessage &msg) {
  if (msg.getInt(0))
    startRoute(1);
}

void route2Callback(OSCMessage &msg) {
  if (msg.getInt(0))
    startRoute(2);
}

void route3Callback(OSCMessage &msg) {
  if (msg.getInt(0))
    startRoute(3);
}

void route4Callback(OSCMessage &msg) {
  if (msg.getInt(0))
    startRoute(4);
}

void route5Callback(OSCMessage &msg) {
  if (msg.getInt(0))
    startRoute(5);
}

void route6Callback(OSCMessage &msg) {
  if (msg.getInt(0))
    startRoute(6);
}

void route7Callback(OSCMessage &msg) {
  if (msg.getInt(0))
    startRoute(7);
}

void route8Callback(OSCMessage &msg) {
  if (msg.getInt(0))
    startRoute(8);
}

void pauseCallback(OSCMessage &msg) {
  if (msg.getInt(0))
    pauseRoute();
}

void resumeCallback(OSCMessage &msg) {
  if (msg.getInt(0))
    resumeRoute();
}

void stopCallback(OSCMessage &msg) {
  if (msg.getInt(0))
    stopRoute();
}

void resetCallback(OSCMessage & msg) {
  if (msg.getInt(0))
    resetRoute();
}

void xCallback(OSCMessage & msg) {
  vx = msg.getFloat(0);
}

void yCallback(OSCMessage & msg) {
  vy = msg.getFloat(0);
}

void aCallback(OSCMessage & msg) {
  va = msg.getFloat(0);
}

void dxCallback(OSCMessage & msg) {
  // adjust the current position with 1 cm
  // the negative adjustment will cause the robot to move towards the positive side
  x -= msg.getFloat(0) * 0.01;
}

void dyCallback(OSCMessage & msg) {
  // adjust the current position with 1 cm
  // the negative adjustment will cause the robot to move towards the positive side
  y -= msg.getFloat(0) * 0.01;
}

void daCallback(OSCMessage & msg) {
  // adjust the current angle with 5 degrees (in radians)
  // the negative adjustment will cause the robot to move towards a positive angle
  a -= msg.getFloat(0) * 5.00 * PI / 180;
}

void xyCallback(OSCMessage & msg) {
  vx = msg.getFloat(0);
  vy = msg.getFloat(1);
}

void accxyzCallback(OSCMessage & msg) {
  // values are between -1 and 1, scale them to a more appropriate speed of 3 cm/s
  vx = msg.getFloat(0) * 0.03;
  vy = msg.getFloat(1) * 0.03;
}
