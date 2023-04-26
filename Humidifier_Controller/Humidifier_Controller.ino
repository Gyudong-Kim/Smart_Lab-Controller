#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h"

#define RELAY D4

const char* ssid = SSID;
const char* password = PASSWORD;
const char* mqtt_server = MQTT_BROKER;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(9600);
  pinMode(RELAY, OUTPUT);
  setup_wifi(); // 와이파이 연결
  client.setServer(mqtt_server, MQTT_PORT); // MQTT 서버 연결
  client.setCallback(callback);
}

void loop() {
  // MQTT 서버 연결이 되지 않았을 때, 재연결 시도
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
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
      Serial.println("Subscribe Topic: humidifier");
      client.subscribe("humidifier");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // 다시 시도하기 전 5초 대기
      delay(5000);
    }
  }
}

void controller(String message) {
  StaticJsonDocument<64> doc;
  deserializeJson(doc, message); // 문자열을 JSON형식으로 변환

  if(doc["code"] == "C_M_003") {
    digitalWrite(RELAY, HIGH);
  } else if (doc["code"] == "C_M_004") {
    digitalWrite(RELAY, LOW);
  } else {
    Serial.println("Received the wrong message");
  }
}