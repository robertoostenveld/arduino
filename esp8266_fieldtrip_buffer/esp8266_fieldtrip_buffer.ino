#include <ESP8266WiFi.h>

#include "secret.h"
#include "fieldtrip_buffer.h"

int ftserver = 0, status = 0;

#define NCHANS    1
#define FSAMPLE   1000
#define BLOCKSIZE 100

#define FTHOST "192.168.1.182"
#define FTPORT 1972

uint16_t buf[BLOCKSIZE];

/**************************************************************************************************/

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ftserver = fieldtrip_open_connection(FTHOST, FTPORT);
  if (ftserver > 0) {
    Serial.println("Connection to FieldTrip buffer server opened");
    status = fieldtrip_write_header(ftserver, DATATYPE_UINT16, NCHANS, FSAMPLE);
    if (status == 0) {
      Serial.println("Wrote header");
    }
    else {
      Serial.println("Failed writing header");
    }
    // status = fieldtrip_close_connection(ftserver);
    status = 0;
    if (status == 0)
      Serial.println("Connection to FieldTrip buffer server closed");
    else
      Serial.println("Failed closing connection");
  }
  else {
    Serial.println("Failed opening connection");
  }
  for (int i = 0; i < BLOCKSIZE; i++)
    buf[i] = 0;
}

/**************************************************************************************************/

void loop() {

  ftserver = fieldtrip_open_connection(FTHOST, FTPORT);
  if (ftserver > 0) {
    Serial.println("Connection to FieldTrip buffer server opened");
    status = fieldtrip_write_data(ftserver, DATATYPE_UINT16, NCHANS, BLOCKSIZE, (byte *)buf);
    if (status == 0)
      Serial.println("Wrote data");
    else
      Serial.println("Failed writing data");
    // status = fieldtrip_close_connection(ftserver);
    status = 0;
    if (status == 0)
      Serial.println("Connection to FieldTrip buffer server closed");
    else
      Serial.println("Failed closing connection");
  }
  else {
    Serial.println("Failed opening connection");
  }

  for (int i = 0; i < BLOCKSIZE; i++)
    buf[i] += 1;

  delay((1000*BLOCKSIZE)/FSAMPLE);
}
