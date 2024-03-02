#include "webinterface.h"
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

bool defaultConfig() {
  config.sensors = 8;
  config.decimate = 1;
  config.calibrate = 0;
  config.raw = 1;
  config.ahrs = 0;
  config.quaternion = 0;
  config.temperature = 0;
  strncpy(config.destination, "192.168.1.34", 32);
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

  N_JSON_TO_CONFIG(sensors, "sensors");
  N_JSON_TO_CONFIG(decimate, "decimate");
  N_JSON_TO_CONFIG(calibrate, "calibrate");
  N_JSON_TO_CONFIG(raw, "raw");
  N_JSON_TO_CONFIG(ahrs, "ahrs");
  N_JSON_TO_CONFIG(quaternion, "quaternion");
  N_JSON_TO_CONFIG(temperature, "temperature");
  S_JSON_TO_CONFIG(destination, "destination");
  N_JSON_TO_CONFIG(port, "port");

  return true;
}

bool saveConfig() {
  Serial.println("saveConfig");
  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  N_CONFIG_TO_JSON(sensors, "sensors");
  N_CONFIG_TO_JSON(decimate, "decimate");
  N_CONFIG_TO_JSON(calibrate, "calibrate");
  N_CONFIG_TO_JSON(raw, "raw");
  N_CONFIG_TO_JSON(ahrs, "ahrs");
  N_CONFIG_TO_JSON(quaternion, "quaternion");
  N_CONFIG_TO_JSON(temperature, "temperature");
  S_CONFIG_TO_JSON(destination, "destination");
  N_CONFIG_TO_JSON(port, "port");

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

void printRequest() {
  String message = "HTTP Request\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nHeaders: ";
  message += server.headers();
  message += "\n";
  for (uint8_t i = 0; i < server.headers(); i++ ) {
    message += " " + server.headerName(i) + ": " + server.header(i) + "\n";
  }
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  Serial.println(message);
}

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
  Serial.print("handleNotFound: ");
  Serial.println(server.uri());
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

void handleRedirect(const char * filename) {
  handleRedirect((String)filename);
}

void handleRedirect(String filename) {
  Serial.println("handleRedirect: " + filename);
  server.sendHeader("Location", filename, true);
  server.send(302, "text/plain", "");
}

bool handleStaticFile(const char * path) {
  return handleStaticFile((String)path);
}

bool handleStaticFile(String path) {
  Serial.println("handleStaticFile: " + path);
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    server.streamFile(file, contentType);               // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}

void handleJSON() {
  // this gets called in response to either a PUT or a POST
  Serial.println("handleJSON");
  printRequest();

  if (server.hasArg("sensors") || server.hasArg("decimate") || server.hasArg("calibrate") || server.hasArg("raw") || server.hasArg("ahrs") || server.hasArg("quaternion") || server.hasArg("temperature") || server.hasArg("destination") || server.hasArg("port")) {
    // the body is key1=val1&key2=val2&key3=val3 and the ESP8266Webserver has already parsed it
    N_KEYVAL_TO_CONFIG(sensors, "sensors");
    N_KEYVAL_TO_CONFIG(decimate, "decimate");
    N_KEYVAL_TO_CONFIG(calibrate, "calibrate");
    N_KEYVAL_TO_CONFIG(raw, "raw");
    N_KEYVAL_TO_CONFIG(ahrs, "ahrs");
    N_KEYVAL_TO_CONFIG(quaternion, "quaternion");
    N_KEYVAL_TO_CONFIG(temperature, "temperature");
    S_KEYVAL_TO_CONFIG(destination, "destination");
    N_KEYVAL_TO_CONFIG(port, "port");

    handleStaticFile("/reload_success.html");
  }
  else if (server.hasArg("plain")) {
    // parse the body as JSON object
    StaticJsonBuffer<300> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
    if (!root.success()) {
      handleStaticFile("/reload_failure.html");
      return;
    }
    N_JSON_TO_CONFIG(sensors, "sensors");
    N_JSON_TO_CONFIG(decimate, "decimate");
    N_JSON_TO_CONFIG(calibrate, "calibrate");
    N_JSON_TO_CONFIG(raw, "raw");
    N_JSON_TO_CONFIG(ahrs, "ahrs");
    N_JSON_TO_CONFIG(quaternion, "quaternion");
    N_JSON_TO_CONFIG(temperature, "temperature");
    S_JSON_TO_CONFIG(destination, "destination");
    N_JSON_TO_CONFIG(port, "port");
    handleStaticFile("/reload_success.html");
  }
  saveConfig();

  // blink five times, this takes 2 seconds
  for (int i = 0; i < 5; i++) {
    ledRed();
    delay(200);
    ledBlack();
    delay(200);
  }

  // some of the settings require re-initialization
  ESP.restart();
}

void handleFileUpload() { // upload a new file to the SPIFFS
  File fsUploadFile;  // a File object to temporarily store the received file
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    Serial.print("handleFileUpload Name: ");
    Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: ");
      Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html");     // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}
