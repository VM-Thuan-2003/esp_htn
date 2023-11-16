#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

typedef struct message {
   bool state_device_1;
   bool state_device_2;
} message;
message myMessage;

void onDataReceiver(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
   Serial.println("Message received.");
   // We don't use mac to verify the sender
   // Let us transform the incomingData into our message structure
  memcpy(&myMessage, incomingData, sizeof(myMessage));
  Serial.print("state_rl_1:");
  Serial.println(myMessage.state_device_1); 
  Serial.print("state_rl_2:");
  Serial.println(myMessage.state_device_2);
}
void setup() {
  Serial.begin(115200);
  WiFi.disconnect();
  ESP.eraseConfig();
 
  // Wifi STA Mode
  WiFi.mode(WIFI_STA);
  // Get Mac Add
  Serial.print("Mac Address: ");
  Serial.print(WiFi.macAddress());
  Serial.println("\nESP-Now Receiver");
  
  // Initializing the ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Problem during ESP-NOW init");
    return;
  }
  
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataReceiver);
}
void loop() {
}