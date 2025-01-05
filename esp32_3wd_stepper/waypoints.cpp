#include "waypoints.h"

// read the sequence of waypoints from the CSV file on SPIFFS
//
// the file should have no header
// each line should consist of four comma-separated values: time, x, y, theta
// time is incremental and in seconds
// x and y are in meter
// theta is in degrees in the file, and converted to radians in memory

vector<float> waypoints_time;
vector<float> waypoints_x;
vector<float> waypoints_y;
vector<float> waypoints_theta;

/****************************************************************************************/

void printWaypoints() {
  Serial.println("printWaypoints");

  for (int i = 0; i < waypoints_time.size(); i++) {
    Serial.print(waypoints_time.at(i));
    Serial.print(",");
    Serial.print(waypoints_x.at(i));
    Serial.print(",");
    Serial.print(waypoints_y.at(i));
    Serial.print(",");
    Serial.print(waypoints_theta.at(i));
    Serial.print("\n");
  }
  return;
}

/****************************************************************************************/

size_t saveWaypoints(String s) {
  Serial.println("saveWaypoints");

  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
    return 0;
  }

  File file = SPIFFS.open("/waypoints.csv", "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
  }
  size_t bytes = file.print(s);
  return bytes;
}

/****************************************************************************************/

String loadWaypoints()
{
  Serial.println("loadWaypoints");

  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
    String s = "";
    return s;
  }

  File file = SPIFFS.open("/waypoints.csv", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
  }

  String s;
  while (file.available())
  {
    char charRead = file.read();
    s += charRead;
  }
  return s;
}

/****************************************************************************************/

void parseWaypoints() {
  Serial.println("parseWaypoints");

  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }

  File file = SPIFFS.open("/waypoints.csv", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  waypoints_time.clear();
  waypoints_x.clear();
  waypoints_y.clear();
  waypoints_theta.clear();

  vector<String> v;
  while (file.available()) {
    String s = file.readStringUntil('\n');
    v.push_back(s);

    int ind1 = s.indexOf(',');                    // finds location of 1st ,
    int ind2 = s.indexOf(',', ind1 + 1);          // finds location of 2nd ,
    int ind3 = s.indexOf(',', ind2 + 1);          // finds location of 3rd ,
    int ind4 = s.length();                        // end of the string

    String tok1 = s.substring(0, ind1);           //captures 1st value
    String tok2 = s.substring(ind1 + 1, ind2);    //captures 2nd value
    String tok3 = s.substring(ind2 + 1, ind3);    //captures 3rd value
    String tok4 = s.substring(ind3 + 1, ind4);    //captures 4th value

    // convert the values to float and store each in their own vector
    waypoints_time.push_back(atof(tok1.c_str()));
    waypoints_x.push_back(atof(tok2.c_str()));
    waypoints_y.push_back(atof(tok3.c_str()));
    waypoints_theta.push_back(atof(tok4.c_str()) * M_PI / 180);  // convert from degrees to radians
  }

  file.close();
  return;
}
