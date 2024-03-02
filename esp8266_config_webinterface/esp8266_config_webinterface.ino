
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#ifndef ARDUINOJSON_VERSION
#error ArduinoJson version 5 not found, please include ArduinoJson.h in your .ino file
#endif

#if ARDUINOJSON_VERSION_MAJOR != 5
#error ArduinoJson version 5 is required
#endif

ESP8266WebServer server(80);
const char* host = "esp8266";
int var1, var2, var3;

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

  printConfig();
  return true;
}

bool saveConfig() {
  Serial.println("saveConfig");
  printConfig();
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
  else {
    root.printTo(configFile);
    return true;
  }
}

void printConfig() {
  Serial.print("var1 = ");
  Serial.println(var1);
  Serial.print("var2 = ");
  Serial.println(var2);
  Serial.print("var3 = ");
  Serial.println(var3);
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
  Serial.println("handleUpdate1");
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  ESP.restart();
}

void handleUpdate2() {
  Serial.println("handleUpdate2");
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
  else {
    Serial.println("\tFile Not Found");
    return false;                                         // If the file doesn't exist, return false
  }
}

void handleJSON() {
  // this gets called in response to either a PUT or a POST
  Serial.println("handleJSON");
  printRequest();

  if (server.hasArg("var1") || server.hasArg("var2") || server.hasArg("var3")) {
    // the body is key1=val1&key2=val2&key3=val3 and the ESP8266Webserver has already parsed it
    String str;
    if (server.hasArg("var1")) {
      str = server.arg("var1");
      var1 = str.toInt();
    }
    if (server.hasArg("var2")) {
      str = server.arg("var2");
      var2 = str.toInt();
    }
    if (server.hasArg("var3")) {
      str = server.arg("var3");
      var3 = str.toInt();
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
    if (root.containsKey("var1"))
      var1 = root["var1"];
    if (root.containsKey("var2"))
      var2 = root["var2"];
    if (root.containsKey("var3"))
      var3 = root["var3"];
    handleStaticFile("/reload_success.html");
  }
  else {
      handleStaticFile("/reload_failure.html");
      return;
  }
  saveConfig();
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  Serial.println("");
  Serial.println("setup");

  SPIFFS.begin();
  loadConfig();

  WiFiManager wifiManager;
  // wifiManager.resetSettings();
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(host);
  Serial.println("connected");

  // this serves all URIs that can be resolved to a file on the SPIFFS filesystem
  server.onNotFound(handleNotFound);

  server.on("/", HTTP_GET, []() {
    handleRedirect("/index.html");
  });

  server.on("/wifi", HTTP_GET, []() {
    handleStaticFile("/reload_success.html");
    Serial.println("handleWifi");
    Serial.flush();
    WiFiManager wifiManager;
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.startConfigPortal(host);
    Serial.println("connected");
  });

  server.on("/reset", HTTP_GET, []() {
    handleStaticFile("/reload_success.html");
    Serial.println("handleReset");
    Serial.flush();
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
  });

  server.on("/dir", HTTP_GET, handleDirList);

  server.on("/json", HTTP_PUT, handleJSON);

  server.on("/json", HTTP_POST, handleJSON);

  server.on("/json", HTTP_GET, [] {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["var1"] = var1;
    root["var2"] = var2;
    root["var3"] = var3;
    String content;
    root.printTo(content);
    server.send(200, "application/json", content);
  });

  server.on("/update", HTTP_GET, []() {
    handleStaticFile("/update.html");
  });

  server.on("/update", HTTP_POST, handleUpdate1, handleUpdate2);

  // ask server to track these headers
  const char * headerkeys[] = {"User-Agent","Content-Type"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize );

  server.begin();

  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);
}

void loop() {
  // put your main code here, to run repeatedly
  server.handleClient();
  delay(10); // in milliseconds
}
