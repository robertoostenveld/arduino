#ifndef _WAYPOINTS_H_
#define _WAYPOINTS_H_

#include <FS.h>
#include <SPIFFS.h>
#include <vector>

using namespace std;

extern vector<float> waypoints_t;
extern vector<float> waypoints_x;
extern vector<float> waypoints_y;
extern vector<float> waypoints_a;

// the following functions take an integer as first input, which is the route number
// the route number is between 1 to 8, so you can use 8 predefined sets of waypoints

void printWaypoints(int);            // print the waypoints on the serial console
void parseWaypoints(int);            // read the waypoints from the file and represent as vectors
String loadWaypoints(int);           // read the waypoints from the file and return as a string
size_t saveWaypoints(int, String);   // save the waypoints (as string) to the file

#endif // _WAYPOINTS_H_
