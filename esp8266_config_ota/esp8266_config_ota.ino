#include <ArduinoJson.h>

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

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
    sendStaticFile(server.uri());
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

void sendRedirect(String filename) {
  char buf[64];
  filename.toCharArray(buf, 64);
  sendRedirect(filename);
}

void sendRedirect(const char * filename) {
  Serial.println("sendRedirect");
  server.sendHeader("Location", String(filename), true);
  server.send(302, "text/plain", "");
}

void sendStaticFile(String filename) {
  char buf[64];
  filename.toCharArray(buf, 64);
  sendStaticFile(buf);
}

void sendStaticFile(const char * filename) {
  Serial.println("sendStaticFile");
  File f = SPIFFS.open(filename, "r");
  Serial.print("Opening file ");
  Serial.print(filename);
  if (!f) {
    Serial.println("  FAILED");
    server.send(500, "text/html", "File not found");
  } else {
    Serial.println("  OK");
    String content = f.readString();
    f.close();
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, getContentType(String(filename)), content);
  }
}

void handleSettings() {
  Serial.println("handleSettings");

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
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
    if (!root.success()) {
      sendStaticFile("/reload_failed.html");
      return;
    }
    if (root.containsKey("var1"))
      var1 = root["var1"];
    if (root.containsKey("var2"))
      var2 = root["var2"];
    if (root.containsKey("var3"))
      var3 = root["var3"];
    sendStaticFile("/reload_success.html");
  }
  else {
    // parse it as key1=val1&key2=val2&key3=val3
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
    sendStaticFile("/reload_success.html");
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
    sendRedirect("/index");
  });

  server.on("/index", HTTP_GET, []() {
    sendStaticFile("/index.html");
  });

  server.on("/wifi", HTTP_GET, []() {
    sendStaticFile("/reload_success.html");
    Serial.println("handleWifi");
    Serial.flush();
    WiFiManager wifiManager;
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.startConfigPortal(host);
    Serial.println("connected");
  });

  server.on("/reset", HTTP_GET, []() {
    sendStaticFile("/reload_success.html");
    Serial.println("handleReset");
    Serial.flush();
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
  });

  server.on("/dir", HTTP_GET, handleDirList);

  server.on("/settings", HTTP_GET, [] {
    sendStaticFile("/settings.html");
  });

  server.on("/json", HTTP_PUT, handleSettings);

  server.on("/json", HTTP_POST, handleSettings);

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
    sendStaticFile("/update.html");
  });

  server.on("/update", HTTP_POST, handleUpdate1, handleUpdate2);

  server.begin();

  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);
}

void loop() {
  // put your main code here, to run repeatedly
  server.handleClient();
  delay(1);
}
