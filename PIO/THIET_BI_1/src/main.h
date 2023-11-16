#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

#include <EEPROM.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <espnow.h>

#include <FirebaseRealtime.h>

// define firebase
#define FIREBASE_REALTIME_URL "https://firebas-htn-default-rtdb.firebaseio.com"
#define FIREBASE_REALTIME_SECRET "hL2w6RHd038c6OUo8axpmD4lfBKGOtebvrgc5idV"

#define EEPROM_SIZE 512 // Kích thước của EEPROM

struct WiFiCredentials {
  char ssid[32];
  char password[32];
};

#define connected 1
#define disconnected 0

#define on 1
#define off 0

#define ssid_ap "IOT_HTN"
#define pass_ap "123456789"

struct messageFire{
    float value_mq2;
    int value_threshold;
    char state_warning[50];
    bool state_control_device_1;
    bool state_control_device_2;
    bool state_connect_wifi;
    bool state_connect_esp;
};

typedef struct messageEsp {
   bool state_device_1;
   bool state_device_2;
} messageEsp;

// define pinOut
#define PIN_MQ2 A0
#define PIN_LED_CONNECT_WIFI A0
#define PIN_LED_CONNECT_ESP A0

#define delay_5s 5000
#define delay_4s 4000
#define delay_3s 3000
#define delay_2s 2000
#define delay_1s 1000

#define CHANNEL 1

// define callback func
void main_init(void);
void clear_eeprom(void);
float read_mq2(void);

bool check_connect_wifi(void);
void reconnect_wifi(void);

void send_firebase(char msg[50], int threshold, float value_mq2);
void control_device(bool dv_1, bool dv_2);
int receive_firebase(void);
void send_esp(void);
#endif