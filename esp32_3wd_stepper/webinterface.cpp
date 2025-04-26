#include "webinterface.h"
#include "waypoints.h"

Config config;
extern WebServer server;

/***************************************************************************/

static String getContentType(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  else if (path.endsWith(".htm")) return "text/html";
  else if (path.endsWith(".css")) return "text/css";
  else if (path.endsWith(".txt")) return "text/plain";
  else if (path.endsWith(".js")) return "application/javascript";
  else if (path.endsWith(".png")) return "image/png";
  else if (path.endsWith(".gif")) return "image/gif";
  else if (path.endsWith(".jpg")) return "image/jpeg";
  else if (path.endsWith(".jpeg")) return "image/jpeg";
  else if (path.endsWith(".ico")) return "image/x-icon";
  else if (path.endsWith(".svg")) return "image/svg+xml";
  else if (path.endsWith(".xml")) return "text/xml";
  else if (path.endsWith(".pdf")) return "application/pdf";
  else if (path.endsWith(".zip")) return "application/zip";
  else if (path.endsWith(".gz")) return "application/x-gzip";
  else if (path.endsWith(".json")) return "application/json";
  return "application/octet-stream";
}

/***************************************************************************/

bool defaultConfig() {
  Serial.println("defaultConfig");

  config.repeat = 0;
  config.absolute = 0;
  config.warp = 1.000;
  config.debug = 0;
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
  N_JSON_TO_CONFIG(absolute, "absolute");
  N_JSON_TO_CONFIG(warp, "warp");
  N_JSON_TO_CONFIG(debug, "debug");

  return true;
}

bool saveConfig() {
  Serial.println("saveConfig");
  JsonDocument root;

  N_CONFIG_TO_JSON(repeat, "repeat");
  N_CONFIG_TO_JSON(absolute, "absolute");
  N_CONFIG_TO_JSON(warp, "warp");
  N_CONFIG_TO_JSON(debug, "debug");

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  } else {
    Serial.println("Writing to config file");
    serializeJson(root, configFile);
    configFile.close();
    return true;
  }
}

void printConfig() {
  Serial.print("repeat = ");
  Serial.println(config.repeat);
  Serial.print("absolute = ");
  Serial.println(config.absolute);
  Serial.print("warp = ");
  Serial.println(config.warp);
  Serial.print("debug = ");
  Serial.println(config.debug);
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
  for (uint8_t i = 0; i < server.headers(); i++) {
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
  } else {
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

void handleRedirect(const char* filename) {
  handleRedirect((String)filename);
}

void handleRedirect(String filename) {
  Serial.println("handleRedirect: " + filename);
  server.sendHeader("Location", filename, true);
  server.setContentLength(0);
  server.send(302, "text/plain", "");
}

bool handleStaticFile(const char* path) {
  return handleStaticFile((String)path);
}

bool handleStaticFile(String path) {
  Serial.println("handleStaticFile: " + path);
  String contentType = getContentType(path);  // Get the MIME type
  if (SPIFFS.exists(path)) {                  // If the file exists
    File file = SPIFFS.open(path, "r");       // Open it
    server.setContentLength(file.size());
    server.streamFile(file, contentType);  // And send it to the client
    file.close();                          // Then close the file again
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;  // If the file doesn't exist, return false
}

void handleJSON() {
  // this gets called in response to either a PUT or a POST
  Serial.println("handleJSON");
  printRequest();

  if (server.hasArg("repeat") || server.hasArg("absolute") || server.hasArg("warp") || server.hasArg("debug")
      || server.hasArg("waypoints1")
      || server.hasArg("waypoints2")
      || server.hasArg("waypoints3")
      || server.hasArg("waypoints4")
      || server.hasArg("waypoints5")
      || server.hasArg("waypoints6")
      || server.hasArg("waypoints7")
      || server.hasArg("waypoints8")) {
    // the body is key1=val1&key2=val2&key3=val3 and the ESP8266Webserver has already parsed it
    N_KEYVAL_TO_CONFIG(repeat, "repeat");
    N_KEYVAL_TO_CONFIG(absolute, "absolute");
    N_KEYVAL_TO_CONFIG(warp, "warp");
    N_KEYVAL_TO_CONFIG(debug, "debug");

    if (server.hasArg("waypoints1"))
      saveWaypoints(1, server.arg("waypoints1"));
    if (server.hasArg("waypoints2"))
      saveWaypoints(2, server.arg("waypoints2"));
    if (server.hasArg("waypoints3"))
      saveWaypoints(3, server.arg("waypoints3"));
    if (server.hasArg("waypoints4"))
      saveWaypoints(4, server.arg("waypoints4"));
    if (server.hasArg("waypoints5"))
      saveWaypoints(5, server.arg("waypoints5"));
    if (server.hasArg("waypoints6"))
      saveWaypoints(6, server.arg("waypoints6"));
    if (server.hasArg("waypoints7"))
      saveWaypoints(7, server.arg("waypoints7"));
    if (server.hasArg("waypoints8"))
      saveWaypoints(8, server.arg("waypoints8"));

    handleStaticFile("/reload_success.html");

  } else if (server.hasArg("plain")) {
    // parse the body as JSON object
    JsonDocument root;
    DeserializationError error = deserializeJson(root, server.arg("plain"));
    if (error) {
      Serial.println("either here");
      handleStaticFile("/reload_failure.html");
      return;
    }
    N_JSON_TO_CONFIG(repeat, "repeat");
    N_JSON_TO_CONFIG(absolute, "absolute");
    N_JSON_TO_CONFIG(warp, "warp");
    N_JSON_TO_CONFIG(debug, "debug");

    if (root.containsKey("waypoints1"))
      saveWaypoints(1, root["waypoints1"]);
    if (root.containsKey("waypoints2"))
      saveWaypoints(2, root["waypoints2"]);
    if (root.containsKey("waypoints3"))
      saveWaypoints(3, root["waypoints3"]);
    if (root.containsKey("waypoints4"))
      saveWaypoints(4, root["waypoints4"]);
    if (root.containsKey("waypoints5"))
      saveWaypoints(5, root["waypoints5"]);
    if (root.containsKey("waypoints6"))
      saveWaypoints(6, root["waypoints6"]);
    if (root.containsKey("waypoints7"))
      saveWaypoints(7, root["waypoints7"]);
    if (root.containsKey("waypoints8"))
      saveWaypoints(8, root["waypoints8"]);

    handleStaticFile("/reload_success.html");

  } else {
    handleStaticFile("/reload_failure.html");
    return;  // do not save the configuration
  }

  saveConfig();
  printConfig();
}
