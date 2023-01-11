#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include "arduino_secrets.h"

#define INFO
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp>
#include "ac_LG.hpp"

#define DHTPIN D2
#define DHTTYPE DHT11

const char* ssid = SSID;
const char* password = PASSWORD;
const char* mqtt_server = MQTT_BROKER;

Aircondition_LG MyLG_Aircondition;
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long time_previous = 0;
char msg[64];

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin(); // 온습도 센서
  setup_wifi(); // 와이파이 연결
  client.setServer(mqtt_server, MQTT_PORT); // MQTT 서버 연결
  client.setCallback(callback);

  // LG 에어컨 IR LED 세팅
  IrSender.begin();
  Serial.print("Ready to send IR signals at Pin: ");
  Serial.println(IR_SEND_PIN);
  MyLG_Aircondition.setType(LG_IS_WALL_TYPE);
}

void loop() {
  // MQTT 서버 연결이 되지 않았을 때, 재연결 시도
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 1초마다 데이터 Publish
  unsigned long time_current = millis();
  if (time_current - time_previous >= 1000) {
    time_previous = time_current;

    // 온습도 데이터를 JSON으로 변환
    serialize_json();

    //Serial.print("Publish message: ");
    //Serial.println(msg);
    client.publish("sensor_data", msg); // sensor_data 토픽에 해당 JSON 데이터를 Publish
  }
}

// 와이파이 연결 함수
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // 와이파이가 연결될 때까지 대기
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String buffer;

  Serial.print("Message arrived '");
  Serial.print(topic);
  Serial.print("': ");

  for (int i = 0; i < length; i++) {
    buffer = buffer + (char)payload[i];
  }
  Serial.println(buffer);
  controller(buffer);
}

// MQTT 서버 재연결 함수
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // 랜덤 client ID 생성
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // MQTT 서버와 연결 시도
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      Serial.println("Subscribe Topic: air_conditioner");
      client.subscribe("air_conditioner");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // 다시 시도하기 전 5초 대기
      delay(5000);
    }
  }
}

// 온습도 데이터를 JSON으로 변환하는 함수
void serialize_json() {
  StaticJsonDocument<64> doc;

  // DHT11 온습도 데이터를 읽음
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  doc["temp"] = round(t);
  doc["humi"] = round(h);
  
  serializeJson(doc, msg); // JSON Serialze
  // Serial.print("JSON data : ");
  // Serial.println(msg);
}

void controller(String message) {
  StaticJsonDocument<64> doc2;
  deserializeJson(doc2, message); // 문자열을 JSON형식으로 변환
  
  if(doc2["code"] == "C_M_001") {        // ON
    int temp = doc2["temp"];
    MyLG_Aircondition.sendCommandAndParameter('1', 0);
    delay(1000);
    MyLG_Aircondition.sendCommandAndParameter('t', temp);
    delay(1000);
    MyLG_Aircondition.sendCommandAndParameter('f', 2);
    Serial.println("===> Air Conditioner Start");
  } else if(doc2["code"] == "C_M_002") { // OFF
    MyLG_Aircondition.sendCommandAndParameter('0', 0);
    Serial.println("===> Air Conditioner Stop");
  } else {
    Serial.println("Received the wrong message");
  }
}