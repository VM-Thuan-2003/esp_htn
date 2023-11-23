#include <ESP8266WiFi.h>
#include <espnow.h>

// #define st_wifi 5
// #define st_esp 6
// #define dev_1 7
// #define dev_2 8

#define del_1 2
#define del_2 4

// Structure to hold the received data
struct __attribute__((packed)) DataStruct {
  bool device_1;
  bool device_2;
};

DataStruct receivedData;

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("DEVICE 1: ");
  Serial.println(receivedData.device_1);
  Serial.print("DEVICE 2: ");
  Serial.println(receivedData.device_2);
}

void setup() {
  Serial.begin(115200);

  // pinMode(dev_1,OUTPUT);
  // pinMode(dev_2,OUTPUT);

  pinMode(del_1,OUTPUT);
  pinMode(del_2,OUTPUT);

  // pinMode(st_wifi,OUTPUT);
  // pinMode(st_esp,OUTPUT);

  WiFi.disconnect();
  // ESP.eraseConfig();
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  Serial.println("Mac Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("\nESP-Now Receiver");
  
  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);

}

void loop() {
  // Do any other processing in the loop if needed
  // digitalWrite(dev_1,receivedData.device_1);
  // digitalWrite(dev_2,receivedData.device_2);
  digitalWrite(del_1,receivedData.device_1);
  digitalWrite(del_2,receivedData.device_2);
}
