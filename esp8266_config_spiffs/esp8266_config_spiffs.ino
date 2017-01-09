#include <ArduinoJson.h>
#include <FS.h>

int var1, var2, var3;

bool loadConfig() {
  Serial.println("loadConfig");
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
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
  Serial.println("saveConfig");
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["var1"] = var1;
  json["var2"] = var2;
  json["var3"] = var3;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFile);
  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  Serial.println("");
  SPIFFS.begin();
  loadConfig();
  Serial.print("var1 = ");
  Serial.println(var1);
  Serial.print("var2 = ");
  Serial.println(var1);
  Serial.print("var3 = ");
  Serial.println(var1);
  saveConfig();

}

void loop() {
}
