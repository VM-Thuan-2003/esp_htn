#include <main.h>

bool state_main = 0;
float dataMq2;
int threshold = 500;

unsigned long time_curr, time_prev;
unsigned long time_wr_curr, time_wr_prev;
unsigned long time_r_curr, time_r_prev;

void setup() {
  main_init();
}

void loop() {
  if(check_connect_wifi() == connected) state_main = 1;
  else{reconnect_wifi(); state_main = 0;}
  if(state_main){
    dataMq2 = read_mq2();
    if(dataMq2 < (float)threshold){
      time_r_curr = millis();
      if(time_r_curr - time_r_prev > delay_3s){
        time_r_prev = time_r_curr;
        threshold = receive_firebase();
      }
      time_curr = millis();
      if(time_curr - time_prev > delay_5s){
        time_prev = time_curr;
        Serial.println("no warning");
        Serial.println("send firebase");
        send_firebase("no warning",threshold,dataMq2);
      }
    }
    else{
      time_wr_curr = millis();
      if(time_curr - time_prev > delay_3s){
        time_prev = time_curr;
        Serial.println("dangered");
        Serial.println("send firebase");
        send_firebase("dangered",threshold,dataMq2);
        // control_device(on,on);
      }
    }
  }
}