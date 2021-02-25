#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266SSDP.h>
#include <SPI.h>
#include <SD.h>

// Web интерфейс для устройства
ESP8266WebServer HTTP(80);

// Для файловой системы
static bool hasSD = false;
File uploadFile;

// Подключаем реле к ногам:
int rele1 = 16; //Pin for reley1
int rele2 = 5; //Pin for reley2

void FSSD_init(void) {
  HTTP.on("/list", HTTP_GET, printDirectory);
  HTTP.on("/edit", HTTP_DELETE, handleDelete);
  HTTP.on("/edit", HTTP_PUT, handleCreate);
  HTTP.on("/edit", HTTP_GET, handleEdit);
  HTTP.on("/users.json", HTTP_GET, usersList);
  HTTP.on("/edit", HTTP_POST, []() {
    returnOK();
  }, handleFileUpload);
  HTTP.onNotFound(handleNotFound);



  if (SD.begin(SS)) {
    Serial.println("SD Card initialized.");
    hasSD = true;
  }
}

void SSDP_init(void) {

  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setName("SDWEBSERVER");
  SSDP.setSerialNumber("001788102201");
  SSDP.setURL("/");
  SSDP.setModelName("SDWEBSERVER");
  SSDP.setModelNumber("000000000001");
  SSDP.setModelURL("https://github.com/vovak2018");
  SSDP.setManufacturer("lava_frai");
  SSDP.setManufacturerURL("https://github.com/vovak2018");
  SSDP.begin();
}

void HTTP_init(void) {
  // Включаем работу с файловой системой
  FSSD_init();

  // SSDP дескриптор
  HTTP.on("/description.xml", HTTP_GET, []() {
    SSDP.schema(HTTP.client());
  });
  //Создание ответа
  HTTP.on("/Reley", handle_Reley); // обрашение к реле через web интерфейс


  // Добавляем функцию Update для перезаписи прошивки
  update();
  // Запускаем HTTP сервер
  HTTP.begin();
}

void returnOK() {
  HTTP.send(200, "text/plain", "");
}

void returnFail(String msg) {
  HTTP.send(500, "text/plain", msg + "\r\n");
}

bool loadFromSdCard(String path) {
  String dataType = "text/plain";
  if (path.endsWith("token.txt")) returnFail("BAD PATH");
  if (path.indexOf("/users/") != -1) {
    Serial.println(path.substring(0, path.lastIndexOf("/")) + "/token.txt");
    if (SD.exists(path.substring(0, path.lastIndexOf("/")) + "/token.txt") && HTTP.args() != 0) {
      String token1 = HTTP.arg(0);
      String token2 = "";
      File tokenFile = SD.open(path.substring(0, path.lastIndexOf("/")) + "/token.txt", FILE_READ);
      while (tokenFile.available()) token2 += char(tokenFile.read());
      tokenFile.close();
      Serial.println(token1 + " ?==? " + token2);
      if (token1 != token2) {
        returnFail("INVALID TOKEN");
        return true;
      }
      //return true;
    }
    else if (path.endsWith("avatar.png")) {
      path = path.substring(0, path.lastIndexOf("/")) + "/data.json";
    }
    else if (path.endsWith("friends.png")) {
      path = path.substring(0, path.lastIndexOf("/")) + "/friends.json";
    }
    else {
      returnFail("NOT AUTHORIZED");
      return true;
    }
  }
  if (path.endsWith("/")) path += "index.html";

  if (path.endsWith(".src")) {
    path = path.substring(0, path.lastIndexOf("."));
    if (HTTP.args() != 0) {
      if (HTTP.hasArg("typeAs")) dataType = HTTP.arg(0);
    }
  }
  else if (path.endsWith(".htm")) dataType = "text/html";
  else if (path.endsWith(".html")) dataType = "text/html";
  else if (path.endsWith(".css")) dataType = "text/css";
  else if (path.endsWith(".js")) dataType = "application/javascript";
  else if (path.endsWith(".png")) dataType = "image/png";
  else if (path.endsWith(".gif")) dataType = "image/gif";
  else if (path.endsWith(".jpg")) dataType = "image/jpeg";
  else if (path.endsWith(".ico")) dataType = "image/x-icon";
  else if (path.endsWith(".xml")) dataType = "text/xml";
  else if (path.endsWith(".pdf")) dataType = "application/pdf";
  else if (path.endsWith(".zip")) dataType = "application/zip";
  else if (path.endsWith(".json")) dataType = "application/json";

  File dataFile = SD.open(path.c_str());
  if (dataFile.isDirectory()) {
    path += "/index.html";
    dataType = "text/html";
    dataFile = SD.open(path.c_str());
  }

  if (!dataFile)
    return false;

  if (HTTP.hasArg("download")) dataType = "application/octet-stream";

  if (HTTP.streamFile(dataFile, dataType) != dataFile.size()) {

  }

  dataFile.close();
  return true;
}

void usersList() {
  //if (!HTTP.hasArg("dir")) return returnFail("BAD ARGS");
  //String path = HTTP.arg("dir");
  //if (path != "/" && !SD.exists((char *)path.c_str())) return returnFail("BAD PATH");
  File dir = SD.open("/users/");
  String path = String();
  if (!dir.isDirectory()) {
    dir.close();
    return returnFail("NOT DIR");
  }
  dir.rewindDirectory();
  HTTP.setContentLength(CONTENT_LENGTH_UNKNOWN);
  HTTP.send(200, "text/json", "");
  WiFiClient client = HTTP.client();

  //HTTP.sendContent("[");
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry)
      break;

    String output;
    if (cnt > 0)
      output = ';';

    //output += "{\"type\":\"";
    //output += (entry.isDirectory()) ? "dir" : "file";
    //output += "\",\"name\":\"";
    if (entry.isDirectory()) output += entry.name();
    //output += "\"";
    //output += "}";
    HTTP.sendContent(output);
    entry.close();
  }
  //HTTP.sendContent("]");
  dir.close();
}

void handleFileUpload() {
  if (HTTP.uri() != "/edit") return;
  HTTPUpload& upload = HTTP.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (SD.exists((char *)upload.filename.c_str())) SD.remove((char *)upload.filename.c_str());
    uploadFile = SD.open(upload.filename.c_str(), FILE_WRITE);

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);

  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();

  }
}

void deleteRecursive(String path) {
  File file = SD.open((char *)path.c_str());
  if (!file.isDirectory()) {
    file.close();
    SD.remove((char *)path.c_str());
    return;
  }

  file.rewindDirectory();
  while (true) {
    File entry = file.openNextFile();
    if (!entry) break;
    String entryPath = path + "/" + entry.name();
    if (entry.isDirectory()) {
      entry.close();
      deleteRecursive(entryPath);
    } else {
      entry.close();
      SD.remove((char *)entryPath.c_str());
    }
    yield();
  }

  SD.rmdir((char *)path.c_str());
  file.close();
}

void handleDelete() {
  if (HTTP.args() == 0) return returnFail("BAD ARGS");
  String path = HTTP.arg(0);
  if (path == "/" || !SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  deleteRecursive(path);
  returnOK();
}

void handleCreate() {
  if (HTTP.args() == 0) return returnFail("BAD ARGS");
  String path = HTTP.arg(0);
  if (path == "/" || SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }

  if (path.indexOf('.') > 0) {
    File file = SD.open((char *)path.c_str(), FILE_WRITE);
    if (file) {
      file.write((const char *)0);
      file.close();
    }
  } else {
    SD.mkdir((char *)path.c_str());
  }
  returnOK();
}

void handleEdit() {
  if (HTTP.args() == 0) return returnFail("BAD ARGS");
  String path = HTTP.arg(0);
  String content = HTTP.arg(1);
  if (path == "/") {
    returnFail("BAD PATH");
    return;
  }

  if (path.indexOf('.') > 0) {
    if (SD.exists((char *)path.c_str())) {
      SD.remove(path);
    }
    File file = SD.open((char *)path.c_str(), FILE_WRITE);
    if (file) {
      int str_len = content.length() + 1; 

      // Подготовьте массив символов (буфер) 
      char char_array[str_len];
      
      // Скопируйте его через
      content.toCharArray(char_array, str_len);
      file.write(char_array);
      file.close();
    }
  } else {
    SD.mkdir((char *)path.c_str());
  }
  returnOK();
}

void printDirectory() {
  if (!HTTP.hasArg("dir")) return returnFail("BAD ARGS");
  String path = HTTP.arg("dir");
  if (path != "/" && !SD.exists((char *)path.c_str())) return returnFail("BAD PATH");
  File dir = SD.open((char *)path.c_str());
  path = String();
  if (!dir.isDirectory()) {
    dir.close();
    return returnFail("NOT DIR");
  }
  dir.rewindDirectory();
  HTTP.setContentLength(CONTENT_LENGTH_UNKNOWN);
  HTTP.send(200, "text/json", "");
  WiFiClient client = HTTP.client();

  HTTP.sendContent("[");
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry)
      break;

    String output;
    if (cnt > 0)
      output = ',';

    output += "{\"type\":\"";
    output += (entry.isDirectory()) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += entry.name();
    output += "\"";
    output += "}";
    HTTP.sendContent(output);
    entry.close();
  }
  HTTP.sendContent("]");
  dir.close();
}

void handleNotFound() {
  if (hasSD && loadFromSdCard(HTTP.uri())) return;
  String message = "SDCARD Not Detected\n\n";
  message += "URI: ";
  message += HTTP.uri();
  message += "\nMethod: ";
  message += (HTTP.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += HTTP.args();
  message += "\n";
  for (uint8_t i = 0; i < HTTP.args(); i++) {
    message += " NAME:" + HTTP.argName(i) + "\n VALUE:" + HTTP.arg(i) + "\n";
  }
  HTTP.send(404, "text/plain", message);

}

void update(void) {
  //Обновление
  HTTP.on("/update", HTTP_POST, []() {
    HTTP.sendHeader("Connection", "close");
    HTTP.sendHeader("Access-Control-Allow-Origin", "*");
    HTTP.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = HTTP.upload();
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
  });
}



void setup() {
  // Настраиваем вывод отладки
  Serial.begin(115200);

  //Включаем WiFiManager
  WiFiManager wifiManager;

  //Если не удалось подключиться клиентом запускаем режим AP
  // доступ к настройкам по адресу http://192.168.4.1
  wifiManager.setWiFiAutoReconnect(true);
  wifiManager.autoConnect("AutoConnectAP");
  wifiManager.setWiFiAutoReconnect(true);
  //если подключение к точке доступа произошло сообщаем
  Serial.println("connected...yeey :)");
  delay(1000);
  // Настраиваем работу реле
  Reley_init();
  Serial.println("RELEY Ready!");

  //настраиваем HTTP интерфейс
  HTTP_init();
  Serial.println("HTTP Ready!");

  //запускаем SSDP сервис
  SSDP_init();
  Serial.println("SSDP Ready!");



}

void loop() {
  // put your main code here, to run repeatedly:
  HTTP.handleClient();
  delay(1);

}
