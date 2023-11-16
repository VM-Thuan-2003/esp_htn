#include <main.h>

bool shouldSaveConfig = false;

bool stateConnectWifi = disconnected;
bool stateConnectEsp = disconnected;

bool control_device_1 = off;
bool control_device_2 = off;

LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiCredentials storedCredentials;

FirebaseRealtime firebaseRealtime;

messageFire payloadFirebase;

messageEsp payloadEsp;

uint8_t peer1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t receiverMacAddress[] = {0x44, 0x17, 0x93, 0x29, 0x56, 0x97};

unsigned long time_read_mq2_curr, time_read_mq2_prev;
float data_read_mq2_prev = 0, data_read_mq2_curr = 0;
unsigned long time_check_connect_curr, time_check_connect_prev;
unsigned long time_reconnect_curr, time_reconnect_prev;
// mini func saveConfigCallback to auto handle when interrupt event setSaveConfigCallback from wifiManager
void saveConfigCallback(void) {
  Serial.println("start save config....");
  shouldSaveConfig = true;
}

void onSent(uint8_t *mac_addr, uint8_t sendStatus) {
    Serial.print("mac: "); Serial.println(*mac_addr);
    Serial.print("Status:"); Serial.println(sendStatus);
}

void main_pinMode(void){
    pinMode( PIN_MQ2, INPUT);
    pinMode( PIN_LED_CONNECT_WIFI, OUTPUT);
    pinMode( PIN_LED_CONNECT_ESP, OUTPUT);
}

void main_state_init(void){
    stateConnectWifi = connected; stateConnectEsp = disconnected;
    control_device_1 = off; control_device_2 = off;
    time_read_mq2_curr = 0; time_read_mq2_prev = 0;
    time_check_connect_curr = 0; time_check_connect_prev = 0;
    time_reconnect_curr = 0; time_reconnect_prev = 0;
}

void main_init(void){
    // declare Serial
    Serial.begin(115200);
    //  declare wifiManager init
    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    //  declare eeprom init
    EEPROM.begin(EEPROM_SIZE); // size of eeprom is 512
    EEPROM.get(0, storedCredentials); // get data from bank 0 of eeprom and save it's value to storedCredentials

    bool isConnect;
    // check data eeprom
    if (strlen(storedCredentials.ssid) > 0 && strlen(storedCredentials.password) > 0){
        // it has data from eeprom
        isConnect = wifiManager.autoConnect(storedCredentials.ssid, storedCredentials.password);
    }
    else{
        // it hasn't data from eeprom
        // start ap wifi -> help user connect to wifi them family
        isConnect = wifiManager.autoConnect(ssid_ap,pass_ap);
    }

    if(isConnect == connected){
        if (shouldSaveConfig) {
            // save information wifi user write to eeprom
            EEPROM.put(0, storedCredentials);
            EEPROM.commit();
            Serial.println("Saving config to EEPROM");
            shouldSaveConfig = false;
        }
        // when esp connected with wifi sta
        Serial.println("Connected to Wi-Fi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("ssid: ");
        Serial.println(storedCredentials.ssid);

        // declare firebaseRealtime when esp connected wifi them family
        firebaseRealtime.begin(FIREBASE_REALTIME_URL, FIREBASE_REALTIME_SECRET, storedCredentials.ssid, storedCredentials.password);

        Serial.print("Mac Address: ");
        Serial.println(WiFi.macAddress());
        Serial.println("ESP-Now Sender");

        WiFi.mode(WIFI_STA);

        // Initializing the ESP-NOW
        if (esp_now_init() != 0) {
            Serial.println("Problem during ESP-NOW init");
            return;
        }
        Serial.println("Registering send callback function");
        esp_now_register_send_cb(onSent);

        // esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
        // Register the peer
        Serial.println("Registering a peer");
        // esp_now_add_peer(peer1, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
        if(esp_now_add_peer(receiverMacAddress, ESP_NOW_ROLE_SLAVE, CHANNEL, NULL, 0) == 0 ){
            Serial.println("add peer is sucessed");
            stateConnectEsp = connected;
        }
        else{
            Serial.println("add peer is failed");
            stateConnectEsp = disconnected;
        }
        payloadFirebase.state_connect_esp = stateConnectEsp;

        lcd.begin(16,2);
        lcd.backlight();
        lcd.setCursor(0,0);
        lcd.print("hello...");
        Serial.println("hello... and start system");
        delay(1000);
        lcd.clear();
        lcd.noBacklight();
        main_pinMode();
        main_state_init();
    }
}

void clear_eeprom(void){
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < EEPROM_SIZE; ++i) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
}

void reconnect_wifi(void){
    time_reconnect_curr = millis();
    if(time_reconnect_curr - time_reconnect_prev > delay_5s){
        time_reconnect_prev = time_reconnect_curr;
        Serial.println("Connection lost. Reconnecting...");
        WiFi.reconnect();
    }
}

bool check_connect_wifi(void){
    time_check_connect_curr = millis();
    if(time_check_connect_curr - time_check_connect_prev > delay_5s){
        time_check_connect_prev = time_check_connect_curr;
        if(WiFi.status() == WL_CONNECTED) stateConnectWifi = connected;
        else stateConnectWifi = disconnected;
    }
    payloadFirebase.state_connect_wifi = stateConnectWifi;
    return stateConnectWifi;
}

float read_mq2(void){
    time_read_mq2_curr = millis();
    if(time_read_mq2_curr - time_read_mq2_prev > delay_3s){
        time_read_mq2_prev = time_read_mq2_curr;
        data_read_mq2_curr = analogRead(PIN_MQ2);
        if(data_read_mq2_prev != data_read_mq2_curr){
            data_read_mq2_prev = data_read_mq2_curr;
        }
    }
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("MQ2 - ");
    lcd.print(data_read_mq2_prev);
    // Serial.print("MQ2 - ");
    // Serial.println(data_read_mq2_prev);
    return data_read_mq2_prev;
}
void send_firebase(char msg[50], int threshold, float value_mq2){
    DynamicJsonDocument saveDoc(1024);
    saveDoc["value_mq2"] = value_mq2;
    saveDoc["value_threshold"] = threshold;
    saveDoc["state_warning"] = (String)msg;
    saveDoc["state_control_device_1"] = control_device_1;
    saveDoc["state_control_device_2"] = control_device_2;
    saveDoc["state_connect_wifi"] = payloadFirebase.state_connect_wifi;
    saveDoc["state_connect_esp"] = payloadFirebase.state_connect_esp;
    String saveJSONData;
    serializeJson(saveDoc, saveJSONData);
    int saveResponseCode = firebaseRealtime.save("esp", "server", saveJSONData);
    Serial.println("\nSave - response code: " + String(saveResponseCode));
    saveDoc.clear();
}
void control_device(bool dv_1, bool dv_2){
    control_device_1 = dv_1;
    control_device_2 = dv_2;
    send_esp();
}
int receive_firebase(void){
    DynamicJsonDocument fetchDoc(1024);
    int fetchResponseCode = firebaseRealtime.fetch("esp", "server", fetchDoc);
    payloadFirebase.value_mq2 = fetchDoc["value_mq2"];
    payloadFirebase.value_threshold = fetchDoc["value_threshold"];
    // payloadFirebase.state_warning = fetchDoc["state_warning"];
    strncpy(payloadFirebase.state_warning, fetchDoc["state_warning"], sizeof(payloadFirebase.state_warning) - 1);
    payloadFirebase.state_control_device_1 = fetchDoc["state_control_device_1"];
    payloadFirebase.state_control_device_2 = fetchDoc["state_control_device_2"];
    payloadFirebase.state_connect_wifi = fetchDoc["state_connect_wifi"];
    payloadFirebase.state_connect_esp = fetchDoc["state_connect_esp"];
    Serial.println("\nFetch - response code: " + String(fetchResponseCode));
    fetchDoc.clear();
    control_device(payloadFirebase.state_control_device_1,payloadFirebase.state_control_device_2);
    return payloadFirebase.value_threshold;
}
void send_esp(void){
    payloadEsp.state_device_1 = control_device_1;
    payloadEsp.state_device_2 = control_device_2;
    esp_now_send(receiverMacAddress, (uint8_t *) &payloadEsp, sizeof(payloadEsp));
}