#include <ESP8266WiFi.h>
#include <espnow.h>

uint8_t receiverMac[] = {0XFF,0XFF,0XFF,0XFF,0XFF,0XFF};

typedef struct {
  char message[50];
} MyData;

MyData myData;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb([](uint8_t* mac, uint8_t sendStatus) {
    if (sendStatus == 0) {
      Serial.println("Message sent successfully");
    } else {
      Serial.println("Error sending message");
    }
  });

  esp_now_add_peer(receiverMac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}

void loop() {
  // Send a message
  strcpy(myData.message, "Hello from Sender");
  esp_now_send(receiverMac, (uint8_t*)&myData, sizeof(myData));

  // Wait for 1 second before receiving
  delay(1000);

  // Your code here
}
