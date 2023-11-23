/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/
#include <WiFi.h>
#include <esp_now.h>

// #define st_wifi 3
// #define st_esp 4
// #define dev_1 5
// #define dev_2 6

#define del_1 GPIO_NUM_22
#define del_2 GPIO_NUM_23

// Structure example to receive data
// Must match the sender structure
// Structure to hold the received data
struct __attribute__((packed)) DataStruct {
  bool device_1;
  bool device_2;
};

DataStruct receivedData;
// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("DEVICE 1: ");
  Serial.println(receivedData.device_1);
  Serial.print("DEVICE 2: ");
  Serial.println(receivedData.device_2);
  // digitalWrite(dev_1,receivedData.device_1);
  // digitalWrite(dev_2,receivedData.device_2);
  digitalWrite(del_1,receivedData.device_1);
  digitalWrite(del_2,receivedData.device_2);
}
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // pinMode(dev_1,OUTPUT);
  // pinMode(dev_2,OUTPUT);

  pinMode(del_1,OUTPUT);
  pinMode(del_2,OUTPUT);

  // pinMode(st_wifi,OUTPUT);
  // pinMode(st_esp,OUTPUT);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
}
void loop() {
  // Do any other processing in the loop if needed
}