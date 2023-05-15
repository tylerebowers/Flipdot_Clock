#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include "RTClib.h"
#include <Wire.h>

#define FLASHTIME 20   //in milliseconds

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
RTC_DS3231 RTC;
DateTime RTCnow;

bool needToSync = true;
short lastMinute = 0;

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
  void idle(){
    digitalWrite(serial, HIGH);
    for(int i = 0; i < 25; i++){
      digitalWrite(SRCLK, HIGH);
      digitalWrite(SRCLK, LOW);
      digitalWrite(RCLK, HIGH);
      digitalWrite(RCLK, LOW);
    }
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
  if (!RTC.begin()) {Serial.println("RTC module missing!");}
  timeClient.setTimeOffset(settings.time_offset);

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
  SR.idle();
  SR.clear();

  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  RTCnow = RTC.now();
}

void flashDisplay(){
  SR.enable();
  delay(FLASHTIME);
  SR.disable();
  SR.idle();
}

void writeTime(short hours, short minutes){
  if(hours > 12){
    Serial.printf("TIME: %d:%d PM\n",hours-12,minutes);
  } else{
    Serial.printf("TIME: %d:%d AM\n",hours,minutes);
  }
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

void loop() {
  if(WiFi.status() == WL_CONNECTED && needToSync){
      timeClient.update();
      RTC.adjust(DateTime(timeClient.getEpochTime()));    //update RTC
      Serial.println(timeClient.getEpochTime());
      Serial.println("Synced RTC!");
      needToSync = false;
  } else if (RTCnow.hour() == 18 && !needToSync){
    needToSync = true;
  } else if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi is NOT connected");
  }
  
  
  while(true){  //wait till next minute
    RTCnow = RTC.now();
    if(RTCnow.minute() != lastMinute){    //update clock
      writeTime(RTCnow.hour(), RTCnow.minute());
      lastMinute = RTCnow.minute();
      break;
    }
    delay(500);
  }
  delay(30000);
}