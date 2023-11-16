#include <ESP8266WiFi.h>
#include <espnow.h>

uint8_t senderMac[] = {0XFF,0XFF,0XFF,0XFF,0XFF,0XFF};

typedef struct {
  char message[50];
} MyData;

MyData receivedData;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
    if (len == sizeof(receivedData)) {
      memcpy(&receivedData, data, sizeof(receivedData));
      Serial.print("Received message: ");
      Serial.println(receivedData.message);
    } else {
      Serial.println("Invalid message size");
    }
  });

  esp_now_add_peer(senderMac, ESP_NOW_ROLE_CONTROLLER, 1, NULL, 0);
}

void loop() {
  // Wait for 1 second before sending
  delay(1000);

  // Send a message
  strcpy(receivedData.message, "Hello from Receiver");
  esp_now_send(senderMac, (uint8_t*)&receivedData, sizeof(receivedData));

  // Your code here
}
