#ifndef _WAYPOINTS_H_
#define _WAYPOINTS_H_

#include <FS.h>
#include <SPIFFS.h>
#include <vector>

using namespace std;

extern vector<float> waypoints_time;
extern vector<float> waypoints_x;
extern vector<float> waypoints_y;
extern vector<float> waypoints_theta;

void readWaypoints();
String waypointsToString();
size_t stringToWaypoints(String);

#endif // _WAYPOINTS_H_
