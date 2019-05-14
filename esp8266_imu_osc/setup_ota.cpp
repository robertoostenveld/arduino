#include "setup_ota.h"
#include "rgb_led.h"

extern ESP8266WebServer server;
extern Config config;
extern unsigned long packetCounter;

/***************************************************************************/

static String getContentType(const String& path) {
  if (path.endsWith(".html"))       return "text/html";
  else if (path.endsWith(".htm"))   return "text/html";
  else if (path.endsWith(".css"))   return "text/css";
  else if (path.endsWith(".txt"))   return "text/plain";
  else if (path.endsWith(".js"))    return "application/javascript";
  else if (path.endsWith(".png"))   return "image/png";
  else if (path.endsWith(".gif"))   return "image/gif";
  else if (path.endsWith(".jpg"))   return "image/jpeg";
  else if (path.endsWith(".jpeg"))  return "image/jpeg";
  else if (path.endsWith(".ico"))   return "image/x-icon";
  else if (path.endsWith(".svg"))   return "image/svg+xml";
  else if (path.endsWith(".xml"))   return "text/xml";
  else if (path.endsWith(".pdf"))   return "application/pdf";
  else if (path.endsWith(".zip"))   return "application/zip";
  else if (path.endsWith(".gz"))    return "application/x-gzip";
  else if (path.endsWith(".json"))  return "application/json";
  return "application/octet-stream";
}

/***************************************************************************/

bool initialConfig() {
  config.sensors = 8;
  config.decimate = 1;
  config.calibrate = 0;
  config.ahrs = 0;
  config.quaternion = 0;
  config.temperature = 0;
  strncpy(config.destination, "192.168.1.182", 32);
  config.port = 8000;
  return true;
}

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
  configFile.close();

  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(buf.get());

  if (!root.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }

  JSON_TO_CONFIG(sensors, "sensors");
  JSON_TO_CONFIG(decimate, "decimate");
  JSON_TO_CONFIG(calibrate, "calibrate");
  JSON_TO_CONFIG(ahrs, "ahrs");
  JSON_TO_CONFIG(quaternion, "quaternion");
  JSON_TO_CONFIG(temperature, "temperature");
  S_JSON_TO_CONFIG(destination, "destination");
  JSON_TO_CONFIG(port, "port");

  return true;
}

bool saveConfig() {
  Serial.println("saveConfig");
  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  CONFIG_TO_JSON(sensors, "sensors");
  CONFIG_TO_JSON(decimate, "decimate");
  CONFIG_TO_JSON(calibrate, "calibrate");
  CONFIG_TO_JSON(ahrs, "ahrs");
  CONFIG_TO_JSON(quaternion, "quaternion");
  CONFIG_TO_JSON(temperature, "temperatures");
  S_CONFIG_TO_JSON(destination, "destination");
  CONFIG_TO_JSON(port, "port");

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }
  else {
    Serial.println("Writing to config file");
    root.printTo(configFile);
    configFile.close();
    return true;
  }
}

/***************************************************************************/

void handleUpdate1() {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  ESP.restart();
}

void handleUpdate2() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.setDebugOutput(true);
    WiFiUDP::stopAll();
    Serial.printf("Update: %s\n", upload.filename.c_str());
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace)) { //start with max available size
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) { //true to set the size to the current progress
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
    Serial.setDebugOutput(false);
  }
  yield();
}

void handleDirList() {
  Serial.println("handleDirList");
  String str = "";
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    str += dir.fileName();
    str += " ";
    str += dir.fileSize();
    str += " bytes\r\n";
  }
  server.send(200, "text/plain", str);
}

void handleNotFound() {
  Serial.println("handleNotFound");
  if (SPIFFS.exists(server.uri())) {
    handleStaticFile(server.uri());
  }
  else {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
  }
}

void handleRedirect(String filename) {
  char buf[64];
  filename.toCharArray(buf, 64);
  handleRedirect(filename);
}

void handleRedirect(const char * filename) {
  Serial.println("handleRedirect");
  server.sendHeader("Location", String(filename), true);
  server.send(302, "text/plain", "");
}

bool handleStaticFile(String path) {
  Serial.println("handleStaticFile");
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}

bool handleStaticFile(const char * path) {
  return handleStaticFile((String)path);
}

void handleJSON() {
  Serial.println("handleJSON");
  String message = "HTTP Request\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  Serial.println(message);

  // this gets called in response to either a PUT or a POST
  if (server.hasArg("plain")) {
    // parse it as JSON object
    StaticJsonBuffer<300> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
    if (!root.success()) {
      handleStaticFile("/reload_failed.html");
      return;
    }
    JSON_TO_CONFIG(sensors, "sensors");
    JSON_TO_CONFIG(decimate, "decimate");
    JSON_TO_CONFIG(calibrate, "calibrate");
    JSON_TO_CONFIG(ahrs, "ahrs");
    JSON_TO_CONFIG(quaternion, "quaternion");
    JSON_TO_CONFIG(temperature, "temperature");
    S_JSON_TO_CONFIG(destination, "destination");
    JSON_TO_CONFIG(port, "port");
    handleStaticFile("/reload_success.html");
  }
  else {
    // parse it as key1=val1&key2=val2&key3=val3
    KEYVAL_TO_CONFIG(sensors, "sensors");
    KEYVAL_TO_CONFIG(decimate, "decimate");
    KEYVAL_TO_CONFIG(calibrate, "calibrate");
    KEYVAL_TO_CONFIG(ahrs, "ahrs");
    KEYVAL_TO_CONFIG(quaternion, "quaternion");
    KEYVAL_TO_CONFIG(temperature, "temperature");
    S_KEYVAL_TO_CONFIG(destination, "destination");
    KEYVAL_TO_CONFIG(port, "port");
    handleStaticFile("/reload_success.html");
  }
  saveConfig();
  // blink five times
  for (int i = 0; i < 5; i++) {
    ledRed();
    delay(200);
    ledBlack();
    delay(200);
  }
  // some of the settings require re-initialization
  ESP.restart();
}
