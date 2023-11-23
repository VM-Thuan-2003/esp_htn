#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <espnow.h>

SoftwareSerial serial_ESP(D3,D4);//D3 = RX -- D4 = TX

// Structure to hold the received data
struct __attribute__((packed)) DataStruct {
  bool device_1;
  bool device_2;
};
DataStruct senderData;

uint8_t peer1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void onSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("mac: "); Serial.println(*mac_addr);
  Serial.print("Status:"); Serial.println(sendStatus);
  serial_ESP.write(sendStatus == 0 ? "done" : "no");
  serial_ESP.write("\n");
}

void setup(){
  pinMode(D3,INPUT);
  pinMode(D4,OUTPUT);

  Serial.begin(115200);
  serial_ESP.begin(9600);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initializing the ESP-NOW
  if (esp_now_init() != 0) {
      Serial.println("Problem during ESP-NOW init");
      return;
  }
  Serial.println("Registering send callback function");
  esp_now_register_send_cb(onSent);
  Serial.println("Registering a peer");
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(peer1, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}
 
void loop(){
  if (serial_ESP.available() > 0) {
      DynamicJsonDocument doc(1024);
      String receivedData = serial_ESP.readStringUntil('\n');
      deserializeJson(doc, receivedData);
      senderData.device_1 = doc["device_1"];
      senderData.device_2 = doc["device_2"];
      Serial.print(senderData.device_1); Serial.print(senderData.device_2); Serial.println("");
      esp_now_send(peer1, (uint8_t *) &senderData, sizeof(senderData));
      doc.clear();
    }
}