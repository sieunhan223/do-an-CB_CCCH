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

// PIN List:
#define SERVOR 15
#define TEMP_HUMI 4
#define RAINSENSOR_ANALOG 34
#define RAINSENSOR_DIGITAL 35
#define TOUCH 0
#define LED 13
// SDA_PIN: 21
// SCL_PIN: 22
// LightSensor I2C Address : 0x23
// PressureSensor I2C Address : 0x77

// DHT Constructor
DHT dht(TEMP_HUMI, DHT22);
float nhiet;
float doam;
String descriptionWeather;
String idWeather;

// create pass and ssid
const char *ssid = "Redmi Note 11";
const char *password = "1122334455";

// Http Constructor
String apiKey = "520a144824bf298dd6a3ab5cf8ab737e";
WiFiClientSecure client;
AsyncWebServer server(80);
HTTPClient http;

// Servo motor constructor
Servo myservo;
int pos;

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

// orther:
int cod;
const char *dateTime_data;
String dateTime;

// Hàm thay thế các Tên trong html file
String processor(const String &var)
{
  if (var == "nhiet")
    return String(int(nhiet));
  if (var == "doam")
    return String(int(doam)) + "%";
  if (var == "id")
    return idWeather;
  if (var == "descriptionWeather")
    return descriptionWeather;
  if (var == "dateTime")
    return dateTime;

  return String();
}
void setup()
{

  Wire.begin();
  Serial.begin(115200);

  // Connect wifi:
  Serial.println("Connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }
  Serial.println("Connected!");
  Serial.print("My IP: ");
  Serial.println(WiFi.localIP());

  // Init SPIFFS:
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Init Servo in 15PIN:
  myservo.attach(SERVOR);

  // Init Light Sensor:
  // LightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23);

  // Init Pressure sensor:
  // Bmp.begin();

  // Init Touch sensor:
  pinMode(TOUCH, INPUT);

  // Init Led:
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
}

void loop()
{
  nhiet = dht.readTemperature();
  doam = dht.readHumidity();
  // lux = LightSensor.readLightLevel();
  // apsuat = Bmp.readPressure();
  touch = digitalRead(TOUCH);
  analogRain = analogRead(RAINSENSOR_ANALOG);
  digitalRain = digitalRead(RAINSENSOR_DIGITAL);

  Serial.print("Nhiet: ");
  Serial.print(nhiet);

  Serial.print("\t");
  Serial.print("Do am: ");
  Serial.print(doam);

  // Serial.print("\t");
  // Serial.print("Light: ");
  // Serial.print(lux);
  // Serial.print(" lx");

  // Serial.print("\t");
  // Serial.print("Pressure = ");
  // Serial.print(apsuat);
  // Serial.print(" Pa");

  Serial.print("\t");
  Serial.print("Touch: ");
  Serial.print(touch);
  if (touch == 1)
    digitalWrite(LED, HIGH);
  else
    digitalWrite(LED, LOW);

  Serial.print("\t");
  Serial.print("analogRain: ");
  Serial.print(analogRain);
  Serial.print("\t");
  Serial.print("digitalRain: ");
  Serial.print(digitalRain);
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
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(cod).c_str());
  http.end();

  // Lấy API từ http://api.openweathermap.org/data/2.5/weather?lat=10.8230&lon=106.6296&appid=8e1880f460a20463565be25bc573bdc6
  http.begin("http://api.openweathermap.org/data/2.5/weather?lat=10.8230&lon=106.6296&appid=8e1880f460a20463565be25bc573bdc6");
  cod = http.GET();
  if (cod == HTTP_CODE_OK)
  {
    String s_weatherApi = http.getString();
    Serial.println(s_weatherApi);
    DynamicJsonDocument doc2(1000);
    DeserializationError error2 = deserializeJson(doc2, s_weatherApi);
    JsonObject obj = doc2.as<JsonObject>();
    String weather = obj["weather"][0]; // lấy dữ liệu tên Data
    deserializeJson(doc2, weather);     // Vì Data là 1 JSON cho nên phải fix data thêm 1 lần nữa
    JsonObject obj2 = doc2.as<JsonObject>();
    String descriptionWeatherg = obj2["description"];
    String idWeatherg = obj2["id"];
    descriptionWeather = descriptionWeatherg;
    idWeather = idWeatherg;
    Serial.println(descriptionWeather);
  }
  else
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(cod).c_str());
  http.end();
  // start server:
  server.begin();
  // Tải nội dung file html
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", String(), false, processor); });
  // Tải nội dung file css
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/style.css", "text/css"); });
  // Tải nội dung file js
  server.on("/weather.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/weather.js", String(), false); });

  // test servo:
  for (pos = 0; pos <= 180; pos += 1)
  { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos); // tell servo to go to position in variable 'pos'
    delay(15);          // waits 15ms for the servo to reach the position
  }
  for (pos = 180; pos >= 0; pos -= 1)
  {                     // goes from 180 degrees to 0 degrees
    myservo.write(pos); // tell servo to go to position in variable 'pos'
    delay(15);          // waits 15ms for the servo to reach the position
  }
}
