// Arduino IDE v1.8.16, v2.2.1
// https://dl.espressif.com/dl/package_esp32_index.json
// http://arduino.esp8266.com/stable/package_esp8266com_index.json
// **Library**
// ESP8266 v3.0.2 , v3.1.2
// AutoConnect v1.3.1 ,v1.4.2
// PageBuilder v1.5.2, v1.5.6
// Firebase Arduino Client Library for ESP8266 and ESP32 v3.3.0
// Rtc By Makuna v2.3.5
// DHT sensor library v1.4.3
// ArduinoJson v6.18.5, v6.21.3
// Adafruit Unified Senser v1.1.5
// TridentTD_LineNotify v3.0.6
// **Fix pyserial : https://forum.arduino.cc/t/pyserial-and-esptools-directory-error/671804/14

// 2.794 millimeter per once
// 60 miilmeter limit
// 20 min cooldwon reset
void ICACHE_RAM_ATTR countWater();

boolean pordMode = true;
// boolean pordMode = false;

//#if defined(ESP32)
//#include <WiFi.h>
//#elif defined(ESP8266)
//#include <ESP8266WiFi.h>
//#endif
#include <Firebase_ESP_Client.h>

// IP 172.217.28.1
#include <AutoConnect.h>
const char* ssid = "Rain Meter (Setting)";
const char* password = "11111111";
AutoConnect Portal;
AutoConnectConfig Config(ssid, password);
#include <ESP8266WiFi.h>

#include "DHT.h"
#define DHTPIN 3
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//Provide the token generation process info.
#include <addons/TokenHelper.h>

//Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
//#define WIFI_SSID "babe"
//#define WIFI_PASSWORD "babe2509"
//For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

/* 2. Define the API Key */
#define API_KEY "AIzaSyA6MFI7sd9MSxkPI3T868jnoDjtHcMXZ7I"

/* 3. Define the RTDB URL */
#define DATABASE_URL "rain-meter-5d6eb-default-rtdb.asia-southeast1.firebasedatabase.app"  //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "cpe.rainmeter@gmail.com"
#define USER_PASSWORD "cpe123456"

// #include <TridentTD_LineNotify.h>
#define LINE_TOKEN "f0E3HB6o7EubJvIVNw8AUC8QpoOeJn7H8sD3mUYm5K1"

//  Rtc DS3231
#include <Wire.h>
#include <RtcDS3231.h>
RtcDS3231<TwoWire> Rtc(Wire);
#define countof(a) (sizeof(a) / sizeof(a[0]))
RtcDateTime now;

// SD Card
#include <SPI.h>
#include <SD.h>

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

// unsigned long currentMillis, previousMillis1, previousMillis2, previousMillis3, previousMillis4, previousMillis5, previousMillis6, previousMillis7, previousMillis8 = 0;
unsigned long currentMillis, previousMillis1, previousMillis3, previousMillis4, previousMillis5, previousMillis6, previousMillis8 = 0;
// Task 1 : Push data to Firebase (Database)
// Task 2 : Delete Data 7 day ago
// Task 3 : Handle WiFi
// Task 4 : Debounce Interrupt
// Task 5 : Read Temp, Hum And Push Data to Firebase
// Task 6 : Reset cooldwon water
// Task 7 : Debounce Line notify
// Task 8 : Send line notify
int t1Interval = pordMode ? 60000 * 10 : 30000;
// int t2Interval = pordMode ? 60000 * 60 : 60000 * 60;
int t3Interval = 6000, t4Interval = 200, t5Interval = 6000;
int t6Interval = pordMode ? 60000 : 2000;
// int t7Interval = pordMode ? 60000 * 20 : 20000;
int t8Interval = pordMode ? 60000 * 15 : 30000;

char dateTimestring[20];
char datestring[11];
String dateTimeNow = "";

uint8_t countWaterPin = D3;
int waterCount = 0;
float waterTotal = 0;
bool counted = false;
const float amountOfWater = 2.794;
int waterLimit = 60;

const int cooldownReset = 20;
int previousWaterCount = 0;
int duplicateCount = 0;
// boolean sendLineNotify = true;

int day = 0;
int month = 0;
int year = 0;

// File myFile;
const int sdCardPin = D4;

int h = 0;
int t = 0;

String req;
// ข้อความ ที่จะแสดงใน Line
String txt1 = "ปริมาณน้ำฝนเกิน 60 มิลลิเมตรแล้ว !";
String txt2 = "ปริมาณน้ำฝนเกิน 90 มิลลิเมตรแล้ว !";
// String line;

String settingTime;
String settingWater;

void setup() {
  //  ปี เดือน วัน ชั่วโมง นาที วินาที
  //  สำหรับตั้งเวลา
  //  RtcDateTime compiled = RtcDateTime(2021, 12, 11, 17, 19, 00);
  //  Rtc.SetDateTime(compiled);
  delay(1000);
  Serial.begin(115200);

  pinMode(countWaterPin, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(countWaterPin), countWater, FALLING);

  if (!SD.begin(sdCardPin)) {
    Serial.println("initialization SD Card failed !");
    //    return;
  }

  Rtc.Begin();
  now = Rtc.GetDateTime();
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

  Serial.println("Starting.....  X)");
  Config.autoReconnect = true;   // Attempt automatic reconnection.
  Config.reconnectInterval = 1;  // Seek interval time is 180[s].
  digitalWrite(LED_BUILTIN, LOW);
  //  Config.ticker = true;
  //  Config.tickerPort = 2;
  //  Config.tickerOn = LOW;
  Portal.config(Config);
  AutoConnectConfig(ssid, password);
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
    digitalWrite(LED_BUILTIN, HIGH);
    //    LINE.notifySticker("เครื่องวัดปริมาณน้ำฝนออนไลน์แล้ว !", 446, 1993);
    //    Config.ticker = false;
  }
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  dht.begin();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  //Or use legacy authenticate method
  //config.database_url = DATABASE_URL;
  //config.signer.tokens.legacy_token = "<database secret>";

  //////////////////////////////////////////////////////////////////////////////////////////////
  //Please make sure the device free Heap is not lower than 80 k for ESP32 and 10 k for ESP8266,
  //otherwise the SSL connection will fail.
  //////////////////////////////////////////////////////////////////////////////////////////////

  Firebase.begin(&config, &auth);

  //Comment or pass false value when WiFi reconnection will control by your code or third party library
  Firebase.reconnectWiFi(false);
  Firebase.setDoubleDigits(5);

  settingTime = Firebase.RTDB.getString(&fbdo, F("/settingTime")) ? fbdo.to<const char*>() : fbdo.errorReason().c_str();
  settingWater = Firebase.RTDB.getString(&fbdo, F("/settingWater")) ? fbdo.to<const char*>() : fbdo.errorReason().c_str();

  // Serial.println(settingTime);
  // Serial.println(settingWater);

  req = "POST /api/notify HTTP/1.1\r\n";
  req += "Host: notify-api.line.me\r\n";
  req += "Authorization: Bearer " + String(LINE_TOKEN) + "\r\n";
  req += "Cache-Control: no-cache\r\n";
  req += "User-Agent: ESP8266\r\n";
  req += "Connection: close\r\n";
  req += "Content-Type: application/x-www-form-urlencoded\r\n";
  req += "Content-Length: " + String(String("message=" + txt1).length()) + "\r\n";
  req += "\r\n";

  if (settingWater == "90") {
    req += "message=" + txt1;
  } else {
    req += "message=" + txt2;
  }

  if (settingTime == "10") {
    t1Interval = pordMode ? 60000 * 10 : 30000;
  } else {
    t1Interval = pordMode ? 60000 * 15 : 30000;
  }
}

void loop() {
  currentMillis = millis();
  if (currentMillis - previousMillis1 >= t1Interval) {
    // if (WiFi.status() == WL_CONNECTED && Firebase.ready() && previousMillis1 >= 10000) {
    if (Firebase.ready() && previousMillis1 >= 10000) {
      Serial.println(F("T1 Run : "));
      waterTotal = waterCount * amountOfWater;
      now = Rtc.GetDateTime();
      dateTimeNow = printDateTime(now);
      json.set("water", waterTotal);
      json.set("time", dateTimeNow);

      Firebase.RTDB.pushJSON(&fbdo, F("/rainmeter/") + printDate(now), &json);
      Serial.println("Push data to firebase");
    }
    previousMillis1 = millis();
  }
  if (currentMillis - previousMillis3 >= t3Interval) {
    // Serial.println("T3 Run : ");
    // t3Callback();
    Serial.print(F("water : "));
    Serial.println(waterTotal);
    Portal.handleClient();
    previousMillis3 = millis();
  }
  if (currentMillis - previousMillis4 >= t4Interval) {
    // Serial.println("T4 Run : ");
    // t4Callback();
    counted = false;
    previousMillis4 = millis();
  }
  if (currentMillis - previousMillis5 >= t5Interval) {
    // Serial.println("T5 Run : ");
    // t5Callback();
    h = dht.readHumidity();
    t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      h = 0;
      t = 0;
    }
    Firebase.RTDB.set(&fbdo, F("/test/temp"), t);
    Firebase.RTDB.set(&fbdo, F("/test/hum"), h);
    previousMillis5 = millis();
  }
  if (currentMillis - previousMillis6 >= t6Interval) {
    // Serial.println("T6 Run : ");
    // t6Callback();
    if (previousWaterCount == waterCount) {
      duplicateCount++;
    } else {
      duplicateCount = 0;
    }
    if (duplicateCount >= cooldownReset) {
      waterCount = 0;
    }
    previousWaterCount = waterCount;
    previousMillis6 = millis();
  }
  // if (currentMillis - previousMillis7 >= t7Interval) {
  // Serial.println("T7 Run : ");
  // t7Callback();
  // previousMillis7 = millis();
  // }
  if (currentMillis - previousMillis8 >= t8Interval && waterTotal >= waterLimit) {
    // Serial.println("T8 Run : ");
    notifyLine();
    previousMillis8 = millis();
  }
}

void notifyLine() {
  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("notify-api.line.me", 443)) {
    Serial.println("ERROR Connection notify-api.line.me failed");
    client.stop();
    return;
  }
  client.print(req);
  // Serial.println(F("[Response:]"));
  // while (client.connected()) {
  //   if (client.available()) {
  //     line = client.readStringUntil('\n');
  //     Serial.println(line);
  //   }
  // }
  client.stop();
  // Serial.println(F("\n[Disconnected]"));
}

String printDateTime(const RtcDateTime& dt) {
  snprintf_P(dateTimestring, countof(dateTimestring), PSTR("%02u-%02u-%04u %02u:%02u:%02u"), dt.Day(), dt.Month(), dt.Year(), dt.Hour(), dt.Minute(), dt.Second());
  return dateTimestring;
}

String printDate(const RtcDateTime& dt) {
  snprintf_P(datestring, countof(datestring), PSTR("%02u-%02u-%04u"), dt.Day(), dt.Month(), dt.Year());
  return datestring;
}

void ICACHE_RAM_ATTR countWater() {
  if (!counted) {
    waterCount++;
    waterTotal = waterCount * amountOfWater;
    counted = true;
  }
}
