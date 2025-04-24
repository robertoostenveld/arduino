#ifndef _UTIL_H_
#define _UTIL_H_

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds

#define SERVO_CONTROL 20  // D9 is GPIO20
#define SERVO_ENABLE 18   // D10 is GPIO18

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>  // https://github.com/knolleary/pubsubclient
#include <ESP32Servo.h>

extern WiFiClient wifi_client;
extern PubSubClient mqtt_client;
extern Servo myservo;

extern RTC_DATA_ATTR struct timeval lastFeed;
extern RTC_DATA_ATTR int feedingHour;
extern RTC_DATA_ATTR int feedingMinute;
extern RTC_DATA_ATTR int feedingAmount;

void mqttCallback(char* topic, byte* payload, unsigned int length);
void moveServo();
long timeDifference(struct timeval t1, struct timeval t2);
void sendMessage(char *address);
void enterDeepSleep(long duration);
void printLocalTime1();
void printLocalTime2();
void printWakeupReason();

#endif
