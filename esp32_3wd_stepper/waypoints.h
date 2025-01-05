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

void printWaypoints();          // print the waypoints on the serial console
void parseWaypoints();          // read the waypoints from the file and represent as vectors
String loadWaypoints();         // read the waypoints from the file and return as a string
size_t saveWaypoints(String);   // save the waypoints (as string) to the file

#endif // _WAYPOINTS_H_
