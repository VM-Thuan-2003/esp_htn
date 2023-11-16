#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <espnow.h>
#include <FirebaseRealtime.h>

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define disconnect 1
#define connected 0

// define pin
#define smokeA0 A0
#define led_state_connect_wifi 11
#define led_state_connect_esp 12

bool state_connect_wifi = disconnect;
bool state_connect_esp = disconnect;

int sensorThres = 400;

// Firebase credentials
#define FIREBASE_REALTIME_URL "https://firebas-htn-default-rtdb.firebaseio.com"
#define FIREBASE_REALTIME_SECRET "hL2w6RHd038c6OUo8axpmD4lfBKGOtebvrgc5idV"

FirebaseRealtime firebaseRealtime;

#define EEPROM_SIZE 512 // Kích thước của EEPROM

struct WiFiCredentials {
  char ssid[32];
  char password[32];
};

WiFiCredentials storedCredentials;

// Flag cho việc lưu cấu hình vào EEPROM
bool shouldSaveConfig = false;

// Flag cho việc xóa EEPROM
bool shouldClearEEPROM = false;

// Biến thời gian cho việc kiểm tra lại kết nối
unsigned long lastConnectionAttemptTime = 0;
const unsigned long connectionAttemptInterval = 5000; // Thử kết nối lại sau mỗi 5 giây

unsigned long lastTimeDipsWifi = 0;
const unsigned long lastTimeDipsWifiInterval = 100;

unsigned long lastTimeDipsEsp = 0;
const unsigned long lastTimeDipsEspInterval = 100;

// Callback thông báo cần lưu cấu hình
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
uint8_t peer1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct message {
   bool state_rl_1;
   bool state_rl_2;
   char state_warning[50];
} message;
struct message myMessage;

void onSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.println("Status:");
  Serial.println(sendStatus);
}

void setup() {
  Serial.begin(115200);
  // Kiểm tra nếu cần xóa EEPROM
  if (shouldClearEEPROM) {
    Serial.println("Clearing EEPROM");
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < EEPROM_SIZE; ++i) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
  }

  // Đọc thông tin Wi-Fi từ EEPROM
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, storedCredentials);

  // Initialize WiFiManager
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // Nếu đã có thông tin Wi-Fi trong EEPROM, sử dụng nó để kết nối
  if (strlen(storedCredentials.ssid) > 0 && strlen(storedCredentials.password) > 0) {
    wifiManager.autoConnect(storedCredentials.ssid, storedCredentials.password);
  } else {
    // Ngược lại, mở trang cấu hình để nhập thông tin Wi-Fi
    wifiManager.autoConnect("IOT_HTN","123456789");
  }

  Serial.println("Connected to Wi-Fi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  firebaseRealtime.begin(FIREBASE_REALTIME_URL, FIREBASE_REALTIME_SECRET, storedCredentials.ssid, storedCredentials.password);

  WiFi.mode(WIFI_STA);
  // Get Mac Add
  Serial.print("Mac Address: ");
  Serial.print(WiFi.macAddress());
  Serial.println("ESP-Now Sender");
  // Initializing the ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Problem during ESP-NOW init");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  // Register the peer
  Serial.println("Registering a peer");
  esp_now_add_peer(peer1, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  // if(esp_now_add_peer(peer1, ESP_NOW_ROLE_SLAVE, 1, NULL, 0) == 0 ){
  //   Serial.println("add peer is sucessed");
  //   state_connect_esp = connected;
  // }
  // else{
  //   Serial.println("add peer is failed");
  //   state_connect_esp = disconnect;
  // }
  Serial.println("Registering send callback function");
  esp_now_register_send_cb(onSent);

  
  lcd.begin();
  // pinMode(smokeA0,INPUT);
  // pinMode(led_state_connect_wifi, OUTPUT);
  // pinMode(led_state_connect_esp, OUTPUT);

}

void loop() {
  // Kiểm tra xem có cần lưu cấu hình không
  if (shouldSaveConfig) {
    // Lưu thông tin Wi-Fi vào EEPROM
    EEPROM.put(0, storedCredentials);
    EEPROM.commit();
    Serial.println("Saving config to EEPROM");
    shouldSaveConfig = false;
  }

  // Kiểm tra kết nối Wi-Fi và kết nối lại nếu cần
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long currConnectionAttemptTime = millis();
    if(currConnectionAttemptTime - lastConnectionAttemptTime > connectionAttemptInterval){
      Serial.println("Connection lost. Reconnecting...");
      WiFi.reconnect();
      lastConnectionAttemptTime = currConnectionAttemptTime; // Cập nhật thời điểm thử kết nối lại
      state_connect_wifi = disconnect;
    }
  }
  else{
    state_connect_wifi = connected;
    read_mq2();
    myMessage.state_rl_1 = 0;
    myMessage.state_rl_2 = 0;
    strcpy(myMessage.state_warning, "no warning");
    Serial.println("Send a new message");
    esp_now_send(NULL, (uint8_t *) &myMessage, sizeof(myMessage));
    delay(5000);
    // save
    DynamicJsonDocument saveDoc(1024);
    saveDoc["name"] = "Device 1";
    saveDoc["temperature"] = 30.00;
    saveDoc["location"][0] = 48.756080;
    saveDoc["location"][1] = 2.302038;
    String saveJSONData;
    serializeJson(saveDoc, saveJSONData);
    int saveResponseCode = firebaseRealtime.save("devices", "1", saveJSONData);
    Serial.println("\nSave - response code: " + String(saveResponseCode));
    saveDoc.clear();
  }
  // log_state_connect();
}
void log_state_connect(){
  if(state_connect_wifi == connected){
    digitalWrite(led_state_connect_wifi, 1);
  }
  else{
    unsigned long currTimeDipsWifi = millis();
    if(currTimeDipsWifi - lastTimeDipsWifi > lastTimeDipsWifiInterval){
      digitalWrite(led_state_connect_wifi, !digitalRead(led_state_connect_wifi));
      lastTimeDipsWifi = currTimeDipsWifi;
    }
  }

  if(state_connect_esp == connected){
    digitalWrite(led_state_connect_esp, 1);
  }
  else{
    unsigned long currTimeDipsEsp = millis();
    if(currTimeDipsEsp - lastTimeDipsEsp > lastTimeDipsEspInterval){
      digitalWrite(led_state_connect_esp, !digitalRead(led_state_connect_esp));
      lastTimeDipsEsp = currTimeDipsEsp;
    }
  }
}
void  read_mq2(){
  int analogSensor = analogRead(smokeA0);
  Serial.print("gia tri: ");
  Serial.print(analogSensor);
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("gia trij mq2: ");
  lcd.setCursor(1,0);
  lcd.print(analogSensor);
}