#include "util.h"

/*****************************************************************************/

void mqttCallback(char* topic, byte* payload, unsigned int length) {

  char buf[256];
  for (int i = 0; i < 256; i++)
    buf[i] = 0;
  for (int i = 0; i < length; i++)
    buf[i] = (char)payload[i];

  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(buf);

  if (strcmp(topic, "feeder/hour")) {
    feedingHour = atol(buf);
  } else if (strcmp(topic, "feeder/minute")) {
    feedingMinute = atol(buf);
  } else if (strcmp(topic, "feeder/amount")) {
    feedingAmount = atol(buf);
  }
}

/*****************************************************************************/

void moveServo() {
  // these are experimentally calibrated
  const int left = 10;
  const int right = 74;
  const int middle = (left + right) / 2;

  myservo.write(middle);
  digitalWrite(SERVO_ENABLE, HIGH);
  for (int posDegrees = middle; posDegrees <= right; posDegrees++) {
    // move to the right to pick up the food
    myservo.write(posDegrees);
    delay(50);
  }
  delay(1000);  // wait for it to drop into the slider
  for (int posDegrees = right; posDegrees >= left; posDegrees--) {
    // move to the left to drop the food
    myservo.write(posDegrees);
    delay(50);
  }
  delay(1000);  // wait for it to drop out of the slider
  for (int posDegrees = left; posDegrees <= middle; posDegrees++) {
    // move back to the middle position
    myservo.write(posDegrees);
    delay(50);
  }
  digitalWrite(SERVO_ENABLE, LOW);
  return;
}

/*****************************************************************************/

long timeDifference(struct timeval t1, struct timeval t2) {
  // compute the time difference in seconds, robust for unsigned integers
  long difference;
  if (t1.tv_sec > t2.tv_sec) {
    difference = t1.tv_sec - t2.tv_sec;
  } else {
    difference = t2.tv_sec - t1.tv_sec;
    difference *= -1;
  }
  return difference;
}

/*****************************************************************************/

void sendMessage(char* topic) {
  // send a MQTT message with the current time to the specified topic
  if (mqtt_client.connected()) {
    char message[40];
    struct tm timeinfo;  // this has multiple fields, nicely organized
    getLocalTime(&timeinfo);
    sprintf(message, "%04d-%02d-%02dT%02d:%02d:%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    Serial.print("Sending MQTT message: ");
    Serial.print(topic);
    Serial.print(" = ");
    Serial.println(message);
    mqtt_client.publish(topic, message);
  } else {
    Serial.println("Cannot send MQTT message");
  }
}

/*****************************************************************************/

void enterDeepSleep(long duration) {
  Serial.print("Going to sleep for ");
  Serial.print(duration);
  Serial.println(" seconds");
  Serial.flush();

  wifi_client.flush();
  wifi_client.stop();

  esp_sleep_enable_timer_wakeup(duration * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

/*****************************************************************************/

void printLocalTime1() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}

/*****************************************************************************/

void printLocalTime2() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL) != 0) {
    Serial.println("Failed to obtain time");
  } else {
    Serial.print("TimeVal-sec = ");
    Serial.print(tv.tv_sec);
    Serial.print(" ");
    Serial.print("TimeVal-usec = ");
    Serial.print(tv.tv_usec);
    Serial.println();
  }
}

/*****************************************************************************/

void printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
    default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}
