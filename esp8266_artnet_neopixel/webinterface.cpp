#include "webinterface.h"

Config config;
extern ESP8266WebServer server;
extern int packetCounter;

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
  Serial.println("defaultConfig");
  
  config.universe = 1;
  config.offset = 0;
  config.pixels = 12;
  config.leds = 4;
  config.white = 0;
  config.brightness = 255;
  config.hsv = 0;
  config.mode = 1;
  config.reverse = 0;
  config.speed = 8;
  config.split = 1;
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

  N_JSON_TO_CONFIG(universe, "universe");
  N_JSON_TO_CONFIG(offset, "offset");
  N_JSON_TO_CONFIG(pixels, "pixels");
  N_JSON_TO_CONFIG(leds, "leds");
  N_JSON_TO_CONFIG(white, "white");
  N_JSON_TO_CONFIG(brightness, "brightness");
  N_JSON_TO_CONFIG(hsv, "hsv");
  N_JSON_TO_CONFIG(mode, "mode");
  N_JSON_TO_CONFIG(reverse, "reverse");
  N_JSON_TO_CONFIG(speed, "speed");
  N_JSON_TO_CONFIG(split, "split");

  return true;
}

bool saveConfig() {
  Serial.println("saveConfig");
  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  N_CONFIG_TO_JSON(universe, "universe");
  N_CONFIG_TO_JSON(offset, "offset");
  N_CONFIG_TO_JSON(pixels, "pixels");
  N_CONFIG_TO_JSON(leds, "leds");
  N_CONFIG_TO_JSON(white, "white");
  N_CONFIG_TO_JSON(brightness, "brightness");
  N_CONFIG_TO_JSON(hsv, "hsv");
  N_CONFIG_TO_JSON(mode, "mode");
  N_CONFIG_TO_JSON(reverse, "reverse");
  N_CONFIG_TO_JSON(speed, "speed");
  N_CONFIG_TO_JSON(split, "split");

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
    server.setContentLength(message.length());
    server.send(404, "text/plain", message);
  }
}

void handleRedirect(const char * filename) {
  handleRedirect((String)filename);
}

void handleRedirect(String filename) {
  Serial.println("handleRedirect: " + filename);
  server.sendHeader("Location", filename, true);
  server.setContentLength(0);
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
    server.setContentLength(file.size());
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

  if (server.hasArg("universe") || server.hasArg("offset") || server.hasArg("pixels") || server.hasArg("leds") || server.hasArg("white") || server.hasArg("brightness") || server.hasArg("hsv") || server.hasArg("mode") || server.hasArg("reverse") || server.hasArg("speed") || server.hasArg("split")) {
    // the body is key1=val1&key2=val2&key3=val3 and the ESP8266Webserver has already parsed it
    N_KEYVAL_TO_CONFIG(universe, "universe");
    N_KEYVAL_TO_CONFIG(offset, "offset");
    N_KEYVAL_TO_CONFIG(pixels, "pixels");
    N_KEYVAL_TO_CONFIG(leds, "leds");
    N_KEYVAL_TO_CONFIG(white, "white");
    N_KEYVAL_TO_CONFIG(brightness, "brightness");
    N_KEYVAL_TO_CONFIG(hsv, "hsv");
    N_KEYVAL_TO_CONFIG(mode, "mode");
    N_KEYVAL_TO_CONFIG(reverse, "reverse");
    N_KEYVAL_TO_CONFIG(speed, "speed");
    N_KEYVAL_TO_CONFIG(split, "split");

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
    N_JSON_TO_CONFIG(universe, "universe");
    N_JSON_TO_CONFIG(offset, "offset");
    N_JSON_TO_CONFIG(pixels, "pixels");
    N_JSON_TO_CONFIG(leds, "leds");
    N_JSON_TO_CONFIG(white, "white");
    N_JSON_TO_CONFIG(brightness, "brightness");
    N_JSON_TO_CONFIG(hsv, "hsv");
    N_JSON_TO_CONFIG(mode, "mode");
    N_JSON_TO_CONFIG(reverse, "reverse");
    N_JSON_TO_CONFIG(speed, "speed");
    N_JSON_TO_CONFIG(split, "split");
    
    handleStaticFile("/reload_success.html");
  }
  else {
    handleStaticFile("/reload_failure.html");
    return; // do not save the configuration
  }
  
  saveConfig();
}
