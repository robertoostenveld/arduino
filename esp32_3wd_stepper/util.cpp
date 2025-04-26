#include "util.h"

/********************************************************************************/

void printMacAddress() {
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  baseMac[0], baseMac[1], baseMac[2],
                  baseMac[3], baseMac[4], baseMac[5]);
  } else {
    Serial.println("Failed to read MAC address");
  }
}


/********************************************************************************/

String getMacAddress() {
  uint8_t baseMac[6];
  String s;
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret != ESP_OK) {
    s += "unknown";
  }
  else {
    for (byte i = 0; i < 6; ++i) {
      char buf[3];
      sprintf(buf, "%02X", baseMac[i]);
      s += buf;
      if (i < 5) s += ':';
    }
  }
  return s;
}


/********************************************************************************/

float maxOfThree(float a, float b, float c) {
  if (a >= b && a >= c) {
    return a;
  }
  else if (b >= a && b >= c) {
    return b;
  }
  else {
    return c;
  }
}
