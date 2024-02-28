#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>

#if defined(ESP32)
#include <SPIFFS.h>
#endif

#ifndef ARDUINOJSON_VERSION
#error ArduinoJson version 5 not found, please include ArduinoJson.h in your .ino file
#endif

#if ARDUINOJSON_VERSION_MAJOR != 5
#error ArduinoJson version 5 is required
#endif

int var1, var2, var3;

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file for reading");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(buf.get());

  if (!root.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }

  if (root.containsKey("var1"))
    var1 = root["var1"];
  if (root.containsKey("var2"))
    var2 = root["var2"];
  if (root.containsKey("var3"))
    var3 = root["var3"];
  return true;
}

bool saveConfig() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["var1"] = var1;
  root["var2"] = var2;
  root["var3"] = var3;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  root.printTo(configFile);
  return true;
}

bool printConfig() {
  Serial.print("var1 = ");
  Serial.println(var1);
  Serial.print("var2 = ");
  Serial.println(var2);
  Serial.print("var3 = ");
  Serial.println(var3);
  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  delay(1000);
  Serial.println("");
  Serial.println("setup start");

  if (SPIFFS.begin()) {
    Serial.println("SPIFFS ok");
  }
  else {
    Serial.println("SPIFFS fail");
  }
  
  if (loadConfig()) {
    Serial.println("loadConfig ok");
  }
  else {
    Serial.println("loadConfig fail");
  }
  
  if (printConfig()) {
      Serial.println("printConfig ok");
  }
  else {
    Serial.println("printConfig fail");
  }

  if (saveConfig()) {
    Serial.println("saveConfig ok");
  }
  else {
    Serial.println("loadConfig fail");
  }

  SPIFFS.end();
  Serial.println("setup end");
}

void loop() {
  delay(1000);
  Serial.print(".");
}
