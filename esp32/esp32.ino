// Lib List
#include <Servo.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "ESPAsyncWebServer.h"
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_BMP085.h>
// #include <WiFiManager.h>

// PIN List:
#define SERVOR 15
#define TEMP_HUMI 4
#define RAINSENSOR_ANALOG 34
#define RAINSENSOR_DIGITAL 35
#define TOUCH 0
#define RELAY 13
// SDA_PIN: 21
// SCL_PIN: 22
// LightSensor I2C Address : 0x23
// PressureSensor I2C Address : 0x77

// input submit constructor
const char *PARAM_INPUT_LIGHT = "lightInput";
const char *PARAM_INPUT_SKYLIGHT = "skylightInput";
const char *PARAM_INPUT_MODE = "modeInput";
String lightStatus = "OFF";
String skylightStatus = "ON";

// input ssid, password config constructor
const char *PARAM_INPUT_SSID = "ssid";
const char *PARAM_INPUT_PWD = "pass";
String ssid;
String password;
// WiFiManager wm;

// DHT Constructor
DHT dht(TEMP_HUMI, DHT22);
float nhiet;
float doam;
String descriptionWeather;
String idWeather;

// Http Constructor
String apiKey = "520a144824bf298dd6a3ab5cf8ab737e";
AsyncWebServer server(80);
HTTPClient http;

// Servo motor constructor
Servo myservo;
int pos;
int curPos;

// Light sensor constructor
BH1750 LightSensor;
float lux;

// pressure sensor constructor
Adafruit_BMP085 Bmp;
float apsuat;

// touch sensor constructor
bool touch = 0;

// rain sensor constructor
double analogRain;
bool digitalRain;
double rainRate;

// orther:
int cod;
const char *dateTime_data;
String dateTime;
int processTime;
String modeColor;
String mode;
bool restart = false;

// Hàm thay thế các Tên trong html file
String processor(const String &var)
{
  if (var == "nhiet")
    return String((nhiet));
  if (var == "doam")
    return String((doam));
  if (var == "id")
    return idWeather;
  if (var == "descriptionWeather")
    return descriptionWeather;
  if (var == "dateTime")
    return dateTime;
  if (var == "rainRate")
    return String(rainRate);
  if (var == "apsuat")
    return String(apsuat);
  if (var == "skylight")
    return String(skylightStatus);
  if (var == "light")
    return String(lightStatus);
  if (var == "modeColor")
    return modeColor;
  if (var == "mode")
    return mode;
  return String();
}

// Hàm đọc file
String readFile(fs::FS &fs, const char *path)
{
  File file = fs.open(path, "r");
  if (!file || file.isDirectory())
  {
    Serial.println("- empty file or failed to open file");
    return String();
  }
  String fileContent;
  while (file.available())
    fileContent = file.readStringUntil('\n');
  return fileContent;
}
// Hàm viết file
void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
    Serial.println("- file written");
  else
    Serial.println("- frite failed");
  file.close();
}

// Hàm sử dụng servo:
void controlServo(String status)
{
  if (status == "ON")
  {
    for (pos = curPos; pos <= 130; pos++)
    {
      myservo.write(pos);
      delay(10);
    }
    curPos = 130;
  }
  else
  {
    for (pos = curPos; pos >= 0; pos--)
    {
      myservo.write(pos);
      delay(10);
    }
    curPos = 0;
  }
}

void setup()
{
  // WiFi.mode(WIFI_STA);
  Wire.begin();
  Serial.begin(115200);

  // Kích hoạt chế độ AP
  Serial.println("Setting AP (Access Point)");
  Serial.print("Configuring access point...");
  WiFi.softAP("Trạm Thời Tiết");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Init SPIFFS:
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Get Value from config files
  ssid = readFile(SPIFFS, "/ssid.txt");
  password = readFile(SPIFFS, "/pwd.txt");
  Serial.println(ssid);
  Serial.println(password);

  // Upload nội dung index2.html
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index2.html","text/html"); });
  server.serveStatic("/", SPIFFS, "/");

  // Nếu có sự kiện cập nhật ssid và pwd:
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
            { 
      int count = request->params();
      for (int i = 0; i < count; i++)
      {
        AsyncWebParameter* param = request->getParam(i);
        //HTTP POST get value ssid
        if (param->name() == PARAM_INPUT_SSID)
          ssid = param->value();
        writeFile(SPIFFS,"/ssid.txt",ssid.c_str());
        //HTTP POST get value password
        if (param->name() == PARAM_INPUT_PWD)
          password = param->value();
        writeFile(SPIFFS,"/pwd.txt",password.c_str());
      }
      restart = true;
      request->send(200, "text/plain", "Done. ESP will restart."); });
  server.begin();

  // Connect wifi
  Serial.println("Connecting...");
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }
  Serial.println("Connected!");
  Serial.print("My IP: ");
  Serial.println(WiFi.localIP());

  //Get value from mode, modeColor file;
  mode = readFile(SPIFFS,"/mode.txt");
  modeColor = readFile(SPIFFS,"/modeColor.txt");

  // Init Servo in 15PIN:
  myservo.attach(SERVOR);

  // Init Light Sensor:
  LightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23);

  // Init Pressure sensor:
  Bmp.begin();

  // Init Touch sensor:
  pinMode(TOUCH, INPUT);

  // Init relay:
  pinMode(RELAY, OUTPUT);
  // digitalWrite(RELAY, HIGH);
  
}

void loop()
{
  if (restart)
  {
    delay(5000);
    ESP.restart();
  }
  nhiet = dht.readTemperature();
  doam = dht.readHumidity();
  lux = LightSensor.readLightLevel();
  apsuat = Bmp.readPressure();
  touch = digitalRead(TOUCH);
  analogRain = analogRead(RAINSENSOR_ANALOG);
  rainRate = 100 - (analogRain / 4095.00) * 100;
  digitalRain = digitalRead(RAINSENSOR_DIGITAL);

  Serial.print("Nhiet: ");
  Serial.print(nhiet);

  Serial.print("\t");
  Serial.print("Do am: ");
  Serial.print(doam);

  Serial.print("\t");
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.print(" lx");

  Serial.print("\t");
  Serial.print("Pressure = ");
  Serial.print(apsuat);
  Serial.print(" Pa");

  Serial.print("\t");
  Serial.print("analogRain: ");
  Serial.print(analogRain);
  Serial.print("\t");
  Serial.print("digitalRain: ");
  Serial.print(digitalRain);
  Serial.print("\t");
  Serial.print("rain rate: ");
  Serial.print(rainRate);
  Serial.println();
  delay(100);

  // Lấy API từ http://worldtimeapi.org/api/timezone/Asia/Ho_Chi_Minh
  http.begin("http://worldtimeapi.org/api/timezone/Asia/Ho_Chi_Minh");
  cod = http.GET();
  if (cod == HTTP_CODE_OK)
  {
    String s_timeApi = http.getString();
    DynamicJsonDocument doc1(1000);
    DeserializationError error1 = deserializeJson(doc1, s_timeApi);
    dateTime_data = doc1["datetime"];

    // format date time:
    String temp = "";
    for (int i = 0; i < strlen(dateTime_data); i++)
    {
      if (dateTime_data[i] == 'T')
        temp += " | ";
      else if (dateTime_data[i] == '.')
        break;
      else if (dateTime_data[i] == '-')
        temp += " ";
      else
        temp += dateTime_data[i];
    }
    dateTime = temp;
  }
  else
    Serial.printf("date time-[HTTP] GET... failed, error: %s\n", http.errorToString(cod).c_str());
  http.end();

  // Lấy API từ http://api.openweathermap.org/data/2.5/weather?lat=10.8230&lon=106.6296&appid=8e1880f460a20463565be25bc573bdc6
  http.begin("http://api.openweathermap.org/data/2.5/weather?lat=10.8230&lon=106.6296&appid=8e1880f460a20463565be25bc573bdc6");
  cod = http.GET();
  if (cod == HTTP_CODE_OK)
  {
    String s_weatherApi = http.getString();
    // Serial.println(s_weatherApi);
    DynamicJsonDocument doc2(1000);
    DeserializationError error2 = deserializeJson(doc2, s_weatherApi);
    JsonObject obj = doc2.as<JsonObject>();
    String weather = obj["weather"][0]; // lấy dữ liệu tên Data
    deserializeJson(doc2, weather);     // Vì Data là 1 JSON cho nên phải fix data thêm 1 lần nữa
    JsonObject obj2 = doc2.as<JsonObject>();
    String descriptionWeatherg = obj2["description"];
    String idWeatherg = obj2["id"];
    descriptionWeather = descriptionWeatherg;
    descriptionWeather.toUpperCase();
    idWeather = idWeatherg;
    Serial.println(descriptionWeather);
  }
  else
    Serial.printf("weather-[HTTP] GET... failed, error: %s\n", http.errorToString(cod).c_str());
  http.end();

  // Upload nội dung file index.html
  server.on("/home", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", String(), false, processor); });
    // Upload nội dung file css
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/style.css", "text/css"); });

  // Nếu có sự kiện change mode:
  server.on("/checked", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
    if (request->hasParam(PARAM_INPUT_MODE)){
      if (mode == "ON"){
        mode = "OFF";
        modeColor = "Red";
        writeFile(SPIFFS,"/mode.txt","OFF");
        writeFile(SPIFFS,"/modeColor.txt","Red");

      }
      else{
        mode = "ON";
        modeColor = "Green";
        writeFile(SPIFFS,"/mode.txt","ON");
        writeFile(SPIFFS,"/modeColor.txt","Green");
      }
    }
    request->redirect("/home"); });

  // Nếu có sự kiện skylight, light:
  server.on("/checkeddivice", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        //Get value light, skylight status:
    if (request->hasParam(PARAM_INPUT_SKYLIGHT)) {
      skylightStatus = request->getParam(PARAM_INPUT_SKYLIGHT)->value();
      if (skylightStatus == "OFF"){
        skylightStatus = "ON";
        controlServo("OFF");
      }
      else if (skylightStatus == "ON"){
        skylightStatus = "OFF";
        controlServo("ON");
      }
      Serial.println(skylightStatus);
    }

    if (request->hasParam(PARAM_INPUT_LIGHT)) {
      lightStatus = request->getParam(PARAM_INPUT_LIGHT)->value();
      if (lightStatus == "OFF"){
        lightStatus = "ON";
        digitalWrite(RELAY,HIGH);         
      }
      else if (lightStatus == "ON"){
        lightStatus = "OFF";
        digitalWrite(RELAY,LOW);
      }
      Serial.println(lightStatus);
    }
    request->redirect("/home"); });

  server.begin();

  // Xử lý:
  if (mode == "ON")
  {
    processTime = dateTime.substring(13, 15).toInt();
    // Serial.println(processTime);
    if (rainRate < 40.0)
    {
      // Xử lý giếng trời
      if ((processTime >= 6 && processTime <= 10) || (processTime >= 16 && processTime <= 22))
      {
        Serial.println("OK");
        skylightStatus = "ON";
        controlServo("ON");
        // delay(10000);
      }
      else
      {
        Serial.println("No OK");
        skylightStatus = "OFF";
        controlServo("OFF");
        // delay(10000);
      }
    }
    else if (rainRate > 40.0)
    {
      Serial.println("No OK");
      skylightStatus = "OFF";
      controlServo("OFF");
      // delay(10000);
    }
    // Xử lý Đèn
    if ((processTime <= 5 && processTime >= 19) || lux < 100.0)
    {
      Serial.println("No OK");
      lightStatus = "ON";
      digitalWrite(RELAY, LOW);
    }
    else
    {
      Serial.println("OK");
      lightStatus = "OFF";
      digitalWrite(RELAY, HIGH);
    }
  }
}
