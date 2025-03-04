#include "webinterface.h"
#include "waypoints.h"

Config config;
extern WebServer server; 

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

  config.repeat = 0;
  config.serialfeedback = 1;
  config.parameter = 0.123;
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

  JsonDocument root;
  DeserializationError error = deserializeJson(root, buf.get());
  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  }

  N_JSON_TO_CONFIG(repeat, "repeat");
  N_JSON_TO_CONFIG(serialfeedback, "serialfeedback");
  N_JSON_TO_CONFIG(parameter, "parameter");

  return true;
}

bool saveConfig() {
  Serial.println("saveConfig");
  JsonDocument root;

  N_CONFIG_TO_JSON(repeat, "repeat");
  N_CONFIG_TO_JSON(serialfeedback, "serialfeedback");
  N_CONFIG_TO_JSON(parameter, "parameter");

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }
  else {
    Serial.println("Writing to config file");
    serializeJson(root, configFile);
    configFile.close();
    return true;
  }
}

void printConfig() {
  Serial.print("repeat = ");
  Serial.println(config.repeat);
  Serial.print("serialfeedback = ");
  Serial.println(config.serialfeedback);
  Serial.print("parameter = ");
  Serial.println(config.parameter);
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
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}

void handleJSON() {
  // this gets called in response to either a PUT or a POST
  Serial.println("handleJSON");
  printRequest();

  if (server.hasArg("repeat") || server.hasArg("serialfeedback") || server.hasArg("parameter") || server.hasArg("waypoints")) {
    // the body is key1=val1&key2=val2&key3=val3 and the ESP8266Webserver has already parsed it
    N_KEYVAL_TO_CONFIG(repeat, "repeat");
    N_KEYVAL_TO_CONFIG(serialfeedback, "serialfeedback");
    N_KEYVAL_TO_CONFIG(parameter, "parameter");

    if (server.hasArg("waypoints")) {
      saveWaypoints(server.arg("waypoints")); // save them to file
      parseWaypoints();                       // load them from file in memory
      printWaypoints();
    }

    handleStaticFile("/reload_success.html");
  }
  else if (server.hasArg("plain")) {
    // parse the body as JSON object
    JsonDocument root;
    DeserializationError error = deserializeJson(root, server.arg("plain"));
    if (error) {
      Serial.println("either here");
      handleStaticFile("/reload_failure.html");
      return;
    }
    N_JSON_TO_CONFIG(repeat, "repeat");
    N_JSON_TO_CONFIG(serialfeedback, "serialfeedback");
    N_JSON_TO_CONFIG(parameter, "parameter");

    if (root.containsKey("waypoints"))
      saveWaypoints(root["waypoints"]);

    handleStaticFile("/reload_success.html");
  }
  else {
    handleStaticFile("/reload_failure.html");
    return; // do not save the configuration
  }

  saveConfig();
  printConfig();
}
