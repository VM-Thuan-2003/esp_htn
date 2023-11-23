#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

#include <ArduinoJson.h>
#include <FirebaseRealtime.h>

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define FIREBASE_REALTIME_URL "https://firebas-htn-default-rtdb.firebaseio.com"
#define FIREBASE_REALTIME_SECRET "hL2w6RHd038c6OUo8axpmD4lfBKGOtebvrgc5idV" 

#define ssid_ap "IOT_HTN"
#define password_ap "123456789"

#define Saved 1

SoftwareSerial serial_ESP(D3,D4);//D3 = RX -- D4 = TX

LiquidCrystal_I2C lcd(0x27,20,4); 

FirebaseRealtime firebaseRealtime;

ESP8266WebServer server(80);

String inputSendr =
      "{\"device_1\":\"true\",\"device_1\":\"true\"}";

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

struct Credentials {
  char username[20];
  char password[20];
  bool stateSave;
};
Credentials storedCredentials;

struct message{
  float value_mq2;
  int value_threshold;
  bool state_warning;
  bool state_control_device_1;
  bool state_control_device_2;
  bool state_connect_wifi;
  bool state_connect_esp;
};
message messageFirebase;


int countDelay = 0;
bool errorConnect = 0;

unsigned long time_curr;
unsigned long time_prev_send_firebase;
unsigned long time_prev_recv_firebase;
unsigned long time_prev_send_esp;

bool st_control_device_1 = false;
bool st_control_device_2 = false;
bool st_connect_wifi = false;
bool st_connect_esp = false;
int vl_threshold = 500;
int value_mq2 = 500;
bool st_warning = false;

#define delay_6s 6000
#define delay_5s 5000
#define delay_4s 4000
#define delay_3s 3000
#define delay_2s 2000
#define delay_1s 1000

void handleRoot() {
  String html = "<h1 style='text-align: center;'>Login Page</h1>";
  html += "<div style='display: flex; justify-content: center;'>";
  html += "<form action='/save' method='post'>";
  html += "  Username: <input type='text' name='username'><br>";
  html += "  Password: <input type='password' name='password'><br>";
  html += "  <input type='submit' value='Save'>";
  html += "</form>";
  html += "</div>";
  server.send(200, "text/html", html);
}

void handleSave() {
  String username = server.arg("username");
  String password = server.arg("password");

  // Ensure username and password are not longer than expected
  username.substring(0, sizeof(storedCredentials.username) - 1).toCharArray(storedCredentials.username, sizeof(storedCredentials.username) - 1);
  password.substring(0, sizeof(storedCredentials.password) - 1).toCharArray(storedCredentials.password, sizeof(storedCredentials.password) - 1);
  
  storedCredentials.stateSave = Saved;
  
  // Write the credentials to EEPROM
  EEPROM.begin(sizeof(Credentials));
  EEPROM.put(0, storedCredentials);
  EEPROM.commit();

  String response = "Credentials saved: " + username + "restart esp ";
  server.send(200, "text/plain", response);
  Serial.println("Resetting ESP8266...");
  ESP.restart();
}

#define pinMq2A A0
#define ledWifi D5
#define ledEsp D6
#define ledWarning D7
#define pinBuzzer D8
// #define pinMq2D D4

void setup_pin(){
  pinMode(pinMq2A,INPUT);
  // pinMode(pinMq2D,INPUT);

  pinMode(ledWifi,OUTPUT);
  pinMode(ledEsp,OUTPUT);
  pinMode(ledWarning,OUTPUT);
  pinMode(pinBuzzer,OUTPUT);

  pinMode(D3,INPUT);
  pinMode(D4,OUTPUT);
}

void setup(){
  Serial.begin(115200);
  serial_ESP.begin(9600);

  lcd.init();                      // initialize the lcd 
  lcd.init();
	lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("hello");
  delay(500);
  lcd.clear();

  setup_pin();

  EEPROM.begin(sizeof(Credentials));
  EEPROM.get(0, storedCredentials);
  EEPROM.end();

  countDelay = 0;
  errorConnect = 0;
  
  WiFi.setOutputPower(19.25);
  if(storedCredentials.stateSave == Saved){
    WiFi.begin(storedCredentials.username, storedCredentials.password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
      if(countDelay < 10) countDelay++;
      else{
        Serial.println("Error connect");
        errorConnect = 1;
        WiFi.softAP(ssid_ap, password_ap);
        Serial.println("Access Point Started");
        Serial.print("IP Address: ");
        Serial.println(WiFi.softAPIP());
        server.on("/", HTTP_GET, handleRoot);
        server.on("/save", HTTP_POST, handleSave);
        server.begin();
        return;
      }
    }
    Serial.println("Connecting is Sussess");
    firebaseRealtime.begin(FIREBASE_REALTIME_URL, FIREBASE_REALTIME_SECRET, storedCredentials.username, storedCredentials.password);
    return;
  }
  WiFi.softAP(ssid_ap, password_ap);
  Serial.println("Access Point Starteprintlnd");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
}

void loop(){
  if(errorConnect == 1){
    server.handleClient();
    st_connect_wifi = false;
  }
  else{
    st_connect_wifi = true;
    time_curr = millis();
    recive_firebase();
    check_connect_esp();
    read_mq2();
    control_device(st_control_device_1,st_control_device_2);
    log_pin();
    log_lcd();
  }
}

void read_mq2(){
  float data = analogRead(pinMq2A);
  value_mq2 = int(data);
  // Serial.println(data);
  if(data > vl_threshold){
    digitalWrite(pinBuzzer,1);
    st_control_device_1 = true;
    st_control_device_2 = true;
    st_warning = true;
    control_device(st_control_device_1,st_control_device_2);
  }
  else{
    st_warning = false;
    digitalWrite(pinBuzzer,0);
  }
  if(time_curr - time_prev_send_firebase > delay_5s){
    time_prev_send_firebase = time_curr;
    send_firebase(data > vl_threshold ? true : false, vl_threshold, data);
  }
}

void recive_firebase(){
  if(time_curr - time_prev_recv_firebase > delay_1s){
    time_prev_recv_firebase = time_curr;
    DynamicJsonDocument fetchDoc(1024);
    int fetchResponseCode = firebaseRealtime.fetch("esp", "server", fetchDoc);
    vl_threshold = fetchDoc["value_threshold"];
    st_control_device_1 = fetchDoc["state_control_device_1"];
    st_control_device_2 = fetchDoc["state_control_device_2"];
    Serial.println("\nFetch - response code: " + String(fetchResponseCode));
    fetchDoc.clear();
  }
}

void send_firebase(bool stateWarning, int threshold, float value_mq2){
    DynamicJsonDocument saveDoc(1024);
    saveDoc["value_mq2"] = value_mq2;
    saveDoc["value_threshold"] = threshold;
    saveDoc["state_warning"] = stateWarning;
    saveDoc["state_control_device_1"] = st_control_device_1;
    saveDoc["state_control_device_2"] = st_control_device_2;
    saveDoc["state_connect_wifi"] = st_connect_wifi;
    saveDoc["state_connect_esp"] = st_connect_esp;
    String saveJSONData;
    serializeJson(saveDoc, saveJSONData);
    int saveResponseCode = firebaseRealtime.save("esp", "server", saveJSONData);
    Serial.println("\nSave - response code: " + String(saveResponseCode));
    saveDoc.clear();
}
void control_device(bool st_dc_1, bool st_dc_2){
  // Serial.print("st_dc_1 - "); Serial.print(st_dc_1); Serial.print("st_dc_2 - "); Serial.println(st_dc_2);
  if(time_curr - time_prev_send_esp > delay_3s){
    time_prev_send_esp = time_curr;
    DynamicJsonDocument doc(512);
    deserializeJson(doc, inputSendr);
    doc["device_1"] = st_dc_1;
    doc["device_2"] = st_dc_2;
    serializeJson(doc, serial_ESP);
    serial_ESP.write("\n");
    doc.clear();
  }
}
void log_pin(){
  Serial.print("ledWifi: "); Serial.print(st_connect_wifi); Serial.print("ledEsp: "); Serial.print(st_connect_esp); Serial.print("ledWarning: "); Serial.print(st_warning);  Serial.println("");
  digitalWrite(ledWifi,st_connect_wifi);
  digitalWrite(ledEsp,st_connect_esp);
  digitalWrite(ledWarning,st_warning);
}
void log_lcd(){
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print("HT. GIAM SAT GAS");
  lcd.setCursor(0,1); lcd.print("GT: "); value_mq2 > 1000 ? lcd.print(value_mq2) : value_mq2 > 100 ? lcd.print(String(value_mq2) + " ") : lcd.print(String(value_mq2) +"  ");
  lcd.setCursor(0,2); lcd.print("NGUONG: "); vl_threshold > 1000 ? lcd.print(vl_threshold) : vl_threshold > 100 ? lcd.print(String(vl_threshold) + " ") : lcd.print(String(vl_threshold) + "  ");
  lcd.setCursor(0,3); lcd.print("ST_1: "); lcd.print(st_control_device_1); lcd.setCursor(9,3); lcd.print("ST_2: "); lcd.print(st_control_device_2);
}
void check_connect_esp(){
  if (serial_ESP.available() > 0) {
    String receivedData = serial_ESP.readStringUntil('\n');
    Serial.println(receivedData == "done" ? "connected esp" : "disconnected esp");
    st_connect_esp = receivedData == "done" ? true : false;
  }
}