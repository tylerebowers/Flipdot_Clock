#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

#define FLASHTIME 2000   //in milliseconds

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -14400);

struct {
  short serial = D8;
  short OE = D7;
  short RCLK = D6;
  short SRCLK = D5;
  short SRCLR = D3;
  void disable() {
    digitalWrite(OE, HIGH); // disable
  }
  void enable(){
    digitalWrite(OE, LOW); // enable
  }
  void clear(){
    digitalWrite(SRCLR, LOW); // clear
    digitalWrite(SRCLR, HIGH); // clear
  }
  void incrementSR(){
    digitalWrite(SRCLK, HIGH);
    digitalWrite(SRCLK, LOW);
  }
  void incrementR(){
    digitalWrite(RCLK, HIGH);
    digitalWrite(RCLK, LOW);
  }
} SR;

void setup() {
  Serial.begin(9600); 
  WiFi.hostname("Flipdot_Clock");
  String inString;
  struct { 
    char wifi_ssid[32] = "";
    char wifi_password[64] = "";
    int time_offset;
    bool left_is_MSD;
  } settings;
  EEPROM.begin(sizeof(settings));
  EEPROM.get(0,settings);

  Serial.println("\nWelcome to the Flipdot Clock Software!");
  Serial.setTimeout(10000);
  Serial.println("Would you like to enter settings? (y/n) (10s timeout)");
  inString = Serial.readStringUntil('\n');
  if (inString[0] == 'y'){
    Serial.setTimeout(30000);
    Serial.printf("Current WiFi settings: {SSID: %s, PASSWORD: %s}\n",settings.wifi_ssid, settings.wifi_password);
    Serial.printf("Current time offset: {OFFSET: %d seconds}\n",settings.time_offset);
    if(settings.left_is_MSD){Serial.printf("Current mode: {MSD: left}\n");} else {Serial.printf("Current mode: {MSD: right}\n");}
    while(true){
      Serial.println("What would you like to change? (wifi/offset/mode/exit)");
      inString = "";
      inString = Serial.readStringUntil('\n');
      if (inString == "wifi"){
        Serial.println("Enter new WIFI ssid:");
        inString = "";
        inString = Serial.readStringUntil('\n');
        if(inString.length() >= 2){
          strcpy(settings.wifi_ssid, inString.c_str());
          Serial.println("Enter new WIFI password:");
          inString = "";
          inString = Serial.readStringUntil('\n');
          Serial.println(inString);
          if(inString.length() >= 2){
            strcpy(settings.wifi_password, inString.c_str());
          }
          Serial.printf("New WiFi settings: {SSID: %s, PASSWORD: %s}\n",settings.wifi_ssid, settings.wifi_password);
        }
      } else if (inString == "offset"){
        Serial.println("Enter new time offset in seconds from UTC:");
        inString = "";
        inString = Serial.readStringUntil('\n');
        settings.time_offset = inString.toInt();
        Serial.printf("New time offset: {OFFSET: %d seconds}\n",settings.time_offset);
      } else if (inString == "mode"){
        Serial.println("Enter what side the most significant digit should be on? (left/right)");
        inString = "";
        inString = Serial.readStringUntil('\n');
        if(inString == "left") {
          settings.left_is_MSD = true;
        } else {
          settings.left_is_MSD = false;
        }
        if(settings.left_is_MSD){Serial.printf("New mode: {MSD: left}\n");} else {Serial.printf("New mode: {MSD: right}\n");}
      } else if (inString == "exit"){
        break;
      } else {
        Serial.println("Please check command spelling.");
      }
    }
    EEPROM.put(0,settings);
    EEPROM.commit();
    Serial.println("Settings saved to flash.");
  } 
  Serial.println("Starting up!");
  WiFi.begin(settings.wifi_ssid, settings.wifi_password);  

  for(int i = 0; i < 25; i++){
    delay(1000);
    if(WiFi.status() == WL_CONNECTED){
      Serial.print("Connection Established! (IP address: ");Serial.print(WiFi.localIP());Serial.println(")");
      timeClient.begin();
      break;
    } else {
      Serial.printf("Attempting Connection (%d)\n",i+1);
    }
  }
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Connection Error!");
  }
  
  pinMode(SR.serial, OUTPUT);
  pinMode(SR.OE, OUTPUT);
  pinMode(SR.RCLK, OUTPUT);
  pinMode(SR.SRCLK, OUTPUT);
  pinMode(SR.SRCLR, OUTPUT);
  SR.disable();
  SR.clear();
}
/*
void flashDisplay(){
  SR.enable();
  delay(FLASHTIME);
  SR.disable();
  SR.clear();
}

void writeTime(char hours, char minutes){
      return;
}

void writeDot(char location, bool state){ // one dot at a time
      SR.disable();
      SR.clear();
      for(short i = 0; i < 12; i++){
        if(i == location && state){
          digitalWrite(SR.serial, HIGH);
          SR.incrementSR();
          SR.incrementR();
          digitalWrite(SR.serial, LOW);
          SR.incrementSR();
          SR.incrementR();
        } else if(i == location && !state){
          digitalWrite(SR.serial, LOW);
          SR.incrementSR();
          SR.incrementR();
          digitalWrite(SR.serial, HIGH);
          SR.incrementSR();
          SR.incrementR();
        } else {
          digitalWrite(SR.serial, HIGH);
          SR.incrementSR();
          SR.incrementR();
          SR.incrementSR();
          SR.incrementR();
        }
      }
      return;
}
*/


void loop() {
  if(WiFi.status() == WL_CONNECTED){
      timeClient.update();
      Serial.print(timeClient.getHours());Serial.print(":");Serial.print(timeClient.getMinutes());Serial.print(":");Serial.println(timeClient.getSeconds());
    }
  delay(2000);
    /*
  for(short i = 0; i < 2; i++){
    for(short j = 0; j < 12; j++){
      Serial.printf("%d:%d\n",j,i);
      writeDot(j, i);
      flashDisplay();
    }
  }
  */

}