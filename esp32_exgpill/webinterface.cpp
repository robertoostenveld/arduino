#include <WebServer.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

#include "webinterface.h"

Config config;
extern WebServer server;
extern unsigned int total;

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

bool defaultConfig() {
  Serial.println("defaultConfig");
  strncpy(config.address, "192.168.1.34", 32);
  config.port = 1972;
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

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(buf.get());

  if (!root.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }

  if (root.containsKey("address"))
    strncpy(config.address, root["address"], 32);
  if (root.containsKey("port"))
    config.port = root["port"];

  printConfig();
  return true;
}

bool saveConfig() {
  Serial.println("saveConfig");
  printConfig();
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["address"] = config.address;
  root["port"] = config.port;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }
  else {
    root.printTo(configFile);
    return true;
  }
}

void printConfig() {
  Serial.print("address = ");
  Serial.println(config.address);
  Serial.print("port = ");
  Serial.println(config.port);
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
  else {
    Serial.println("\tFile Not Found");
    return false;                                         // If the file doesn't exist, return false
  }
}

void handleJSON() {
  // this gets called in response to either a PUT or a POST
  Serial.println("handleJSON");
  printRequest();

  if (server.hasArg("address") || server.hasArg("port") || server.hasArg("var1") || server.hasArg("var2") || server.hasArg("var3")) {
    // the body is key1=val1&key2=val2&key3=val3 and the Webserver has already parsed it
    String str;
    if (server.hasArg("address")) {
      str = server.arg("address");
      strncpy(config.address, str.c_str(), 32);
    }
    if (server.hasArg("port")) {
      str = server.arg("port");
      config.port = str.toInt();
    }
    handleStaticFile("/reload_success.html");
  }
  else if (server.hasArg("plain")) {
    // parse the body as JSON object
    String body = server.arg("plain");
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(body);
    if (!root.success()) {
      handleStaticFile("/reload_failure.html");
      return;
    }
    if (root.containsKey("address"))
      strncpy(config.address, root["address"], 32);
    if (root.containsKey("port"))
      config.port = root["port"];
    handleStaticFile("/reload_success.html");
  }
  else {
    handleStaticFile("/reload_failure.html");
    return;
  }
  
  saveConfig();
}
