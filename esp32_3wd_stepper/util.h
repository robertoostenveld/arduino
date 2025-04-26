#ifndef _UTIL_H_
#define _UTIL_H_

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

void printMacAddress(void);
String getMacAddress(void);
float maxOfThree(float, float, float);

#endif // _UTIL_H_
