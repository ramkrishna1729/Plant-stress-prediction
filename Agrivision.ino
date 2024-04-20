#include <ESP32Servo.h>
#include "DHT.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>


#define DHTTYPE DHT11
#define DHTPIN 13
#define BLYNK_TEMPLATE_ID "TMPL3Z3nJxn27"
#define BLYNK_TEMPLATE_NAME "Smart Irrigation"
#define BLYNK_AUTH_TOKEN "Jw0rHWQxeu9T5c_Bd6AS44OD-sX4X7Ba"

#define BLYNK_POINT Serial

#include <BlynkSimpleEsp32.h>
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Hunter";
char pass[] = "qwertyuiop";

LiquidCrystal_I2C lcd(0x27, 16, 2);

BlynkTimer timer;

Servo myservo;  // create servo object to control a servo
int servoPin = 14; // Pin to which the servo is connected
int pos = 0;
int light = 5;
const int pump = 4;
int ldr_pin = 32;


DHT dht(DHTPIN, DHTTYPE);
const char *serverName = "http://api.thingspeak.com/update";
String apiKey = "FIYN38D3UTXG2S6A";



const int m1_pin = 33;
const int m2_pin = 34;
const int m3_pin = 35;


float m1_moi = 0.0;
float m2_moi = 0.0;
float m3_moi = 0.0;
int ldr_val = 1900;
int threshold = 30;
float h = 0.0;
float t = 0.0;
int pump_flag = 0;
int automatic_flag = 1;

void sendMoisture1() {
  if (m1_moi < 30) {
    Serial.print(m1_moi);
    Blynk.logEvent("moisture_1", "moisture is less in sensor 1");
  }
}

void sendMoisture2() {
  if (m2_moi < 30) {
    Serial.print(m2_moi);
    Blynk.logEvent("moisture_2", "moisture is less in sensor 2");
  }
}

void sendMoisture3() {
  if (m3_moi < 30) {
    Serial.print(m3_moi);
    Blynk.logEvent("moisture_3", "moisture is less in sensor 3");
  }
}

void sendLdr() {
  if (ldr_val < 1000) {
    Blynk.logEvent("ldr", "Light is low");
  }
}

void sendHumidity() {
  if (h >= 90.0) {
    Blynk.logEvent("humidity", "Humidity is very high");
  }
}

BLYNK_WRITE(V0) {
  pump_flag = param.asInt();
  Serial.print("Pump flag is ");
  Serial.println(pump_flag);
}

BLYNK_WRITE(V1) {
  automatic_flag = param.asInt();
  Serial.print("Automatic Flag is ");
  Serial.println(automatic_flag);
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.clear();
  lcd.backlight();
  WiFi.begin(ssid, pass);
  dht.begin();
  Serial.println("Connecting");

  while (WiFi.status() != WL_CONNECTED)
  {

      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");    Serial.println(WiFi.localIP());


  lcd.setCursor(0,0);

  myservo.attach(servoPin);
  // pinMode(datapin, OUTPUT);
  pinMode(pump, OUTPUT);
  pinMode(light, OUTPUT);
  myservo.write(pos);

  Blynk.begin(auth, ssid, pass);
  timer.setInterval(5000L, sendMoisture1);
  timer.setInterval(5000L, sendMoisture2);
  timer.setInterval(5000L, sendMoisture3);
  timer.setInterval(5000L, sendLdr);
  timer.setInterval(5000L, sendHumidity);


}

void loop() {
  Blynk.run();
  timer.run();
  // getting the value of moisture 
  float m1_val = analogRead(m1_pin);
  float m2_val = analogRead(m2_pin);
  float m3_val = analogRead(m3_pin);
  
  m1_moi = moisture_val(m1_val, 1);
  m2_moi = moisture_val(m2_val, 2);
  m3_moi = moisture_val(m3_val, 3);
  ldr_val = analogRead(ldr_pin);

  WiFiClient client;
  HTTPClient http;
  h = dht.readHumidity();
  t = dht.readTemperature();
  http.begin(client, serverName);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String httpRequestData = "api_key=" + apiKey + "&field1=" + String(t) + "&field2=" + String(h) + "&field3=" + String(ldr_val) + "&field4=" + String(m1_moi)  + "&field5=" + String(m2_moi) + "&field6=" + String(m3_moi);
  int httpResponseCode = http.POST(httpRequestData);
  Serial.println("humidity, temperature");
  Serial.print(h);
  Serial.print(" ");
  Serial.print(t);
  Serial.println(httpResponseCode);

  lcd.setCursor(0, 0);
  lcd.print(m1_moi);
  lcd.print(" ");
  lcd.setCursor(8, 0);
  lcd.print(m2_moi);
  lcd.setCursor(0, 1);
  lcd.print(m3_moi);
  lcd.setCursor(8, 1);
  lcd.print(ldr_val);
  lcd.setCursor(13, 0);
  lcd.print(t);
  lcd.setCursor(13, 1);
  lcd.print(h);

  if (automatic_flag == 1) {
    while (m1_moi < threshold) {
      myservo.write(0);
      digitalWrite(pump, HIGH);
      m1_moi = analogRead(m1_pin);
      m1_moi = moisture_val(m1_moi, 1);
      delay(1000);
    }
    digitalWrite(pump, LOW);

    while (m2_moi < threshold) {
      myservo.write(90);
      digitalWrite(pump, HIGH);
      m2_moi = analogRead(m2_pin);
      m2_moi = moisture_val(m2_moi, 2);
      delay(1000);
    }
    digitalWrite(pump, LOW);

    while (m3_moi < threshold) {
      myservo.write(180);
      digitalWrite(pump, HIGH);
      m3_moi = analogRead(m3_pin);
      m3_moi = moisture_val(m3_moi, 3);
      delay(1000);
    }
    digitalWrite(pump, LOW);
  } else {
    if (pump_flag == 1) {
      digitalWrite(pump, HIGH);
    } else {
      digitalWrite(pump, LOW);
    }
  }
  delay(1000);
}


float moisture_val(float val, int num) {
  float _moisture =(100 - ((val/4095.00)*100));
  Serial.print("Moisture Percentange ");
  Serial.print(num);
  Serial.print(" Value: ");
  Serial.println(_moisture);
  return _moisture;
}