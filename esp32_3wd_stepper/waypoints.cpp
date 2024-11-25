#include "waypoints.h"

// read the pattern of waypoints from the CSV file on SPIFFS
//
// the file should have no header
// each line should consist of four comma-separated values: time, x, y, theta
// time is incremental and in seconds
// x and y are in meter
// theta is in degrees (and will be converted to radians on the fly)

vector<float> waypoints_time;
vector<float> waypoints_x;
vector<float> waypoints_y;
vector<float> waypoints_theta;

/****************************************************************************************/

size_t saveWaypoints(String s) {
  Serial.println("saveWaypoints");

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
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  File file = SPIFFS.open("/waypoints.csv", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  vector<String> v;
  while (file.available()) {
    String s = file.readStringUntil('\n');
    v.push_back(s);

    int ind1 = s.indexOf(',');                    // finds location of 1st ,
    int ind2 = s.indexOf(',', ind1 + 1);          // finds location of 2nd ,
    int ind3 = s.indexOf(',', ind2 + 1);          // finds location of 3rd ,
    int ind4 = s.length();                        // end of the string

    String time = s.substring(0, ind1 - 1);          //captures 1st value
    String x = s.substring(ind1 + 1, ind2 - 1);      //captures 2nd value
    String y = s.substring(ind2 + 1, ind3 - 1);      //captures 3rd value
    String theta = s.substring(ind3 + 1, ind4 - 1);  //captures 4th value

    // convert the values to float and store each in their own vector
    waypoints_time.push_back(atof(time.c_str()));
    waypoints_x.push_back(atof(x.c_str()));
    waypoints_y.push_back(atof(y.c_str()));
    waypoints_theta.push_back(atof(theta.c_str()));
  }
  file.close();

  for (String s : v) {
    Serial.println(s);
  }
}
