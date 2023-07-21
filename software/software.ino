#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include "RTClib.h"
#include <Wire.h>

#define FLASHTIME 8   //in milliseconds

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
RTC_DS3231 RTC;
DateTime RTCnow;
short shownDisplay = 0;
short shownMinute = -1;

struct { 
  char wifi_ssid[32] = "";
  char wifi_password[64] = "";
  int time_offset;
  int flip_delay;
  bool left_is_MSD;
  bool active_hours_enable;
  int active_lowerbound;
  int active_upperbound;
} settings;

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
  pinMode(SR.serial, OUTPUT);
  pinMode(SR.OE, OUTPUT);
  pinMode(SR.RCLK, OUTPUT);
  pinMode(SR.SRCLK, OUTPUT);
  pinMode(SR.SRCLR, OUTPUT);
  SR.disable();
  SR.clear();
  Serial.begin(9600); 
  WiFi.hostname("Flipdot_Clock");
  EEPROM.begin(sizeof(settings));
  EEPROM.get(0,settings);
  String inString;

  Serial.setTimeout(10000);
  Serial.println("\nWelcome to the Flipdot Clock Software!");
  Serial.printf("  Current WiFi settings: {SSID: %s, PASSWORD: %s}\n",settings.wifi_ssid, settings.wifi_password);
  if(settings.active_hours_enable){Serial.printf("  Current active hours: {ENABLE: true, START: %d, STOP: %d}\n",settings.active_lowerbound,settings.active_upperbound);}else{Serial.printf("  Current active hours: {ENABLE: false}\n");}
  Serial.printf("  Current time offset: {OFFSET: %d seconds}\n",settings.time_offset);
  Serial.printf("  Current flip delay: {DELAY: %d milliseconds}\n",settings.flip_delay);
  if(settings.left_is_MSD){Serial.printf("  Current mode: {MSD: left}\n");} else {Serial.printf("  Current mode: {MSD: right}\n");}
  Serial.println("Would you like to enter settings? (y/n) (10s timeout)");
  
  inString = Serial.readStringUntil('\n');
  if (inString[0] == 'y'){
    Serial.setTimeout(30000);
    while(true){
      Serial.println("What would you like to change? (wifi/offset/delay/mode/active/exit)");
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
          } else {
            Serial.println("PASSWORD was not long enough!");
          }
          Serial.printf("  New WiFi settings: {SSID: %s, PASSWORD: %s}\n",settings.wifi_ssid, settings.wifi_password);
        } else {
          Serial.println("SSID was not long enough!");
        }
      } else if (inString == "offset"){
        Serial.println("Enter new time offset in seconds from UTC:");
        inString = "";
        inString = Serial.readStringUntil('\n');
        settings.time_offset = inString.toInt();
        Serial.printf("  New time offset: {OFFSET: %d seconds}\n",settings.time_offset);
      } else if (inString == "delay"){
        Serial.println("It is recommended that the flip delay is between 50ms and 500ms.");
        Serial.println("Enter new flip delay in milliseconds:");
        inString = "";
        inString = Serial.readStringUntil('\n');
        settings.flip_delay = inString.toInt();
        Serial.printf("  New flip delay: {DELAY: %d milliseconds}\n",settings.flip_delay);
      } else if (inString == "mode"){
        Serial.println("Enter what side the most significant digit should be on? (left/right)");
        inString = "";
        inString = Serial.readStringUntil('\n');
        if(inString == "left") {
          settings.left_is_MSD = true;
        } else {
          settings.left_is_MSD = false;
        }
        if(settings.left_is_MSD){Serial.printf("  New mode: {MSD: left}\n");} else {Serial.printf("  New mode: {MSD: right}\n");}
      } else if (inString == "active") {
        Serial.println("Active hours control when the dots can flip. Important note: lowerbound MUST be less than upperbound. (active hours cannot be interday, intraday only)");
        Serial.println("Enter if active hours should be enabled? (true/false)");
        inString = "";
        inString = Serial.readStringUntil('\n');
        if(inString == "true") {
          settings.active_hours_enable = true;
        } else {
          settings.active_hours_enable = false;
        }
        if(settings.active_hours_enable){
          Serial.println("Enter the LOWER bound hour (when to start active hours)? (0-23)");
          inString = "";
          inString = Serial.readStringUntil('\n');
          if(inString.toInt() < 24 && inString.toInt() >= 0){
            settings.active_lowerbound = inString.toInt();
          } else {
            Serial.println("The number you entered was out of range (0-23).");
          }
          Serial.println("Enter the UPPER bound hour (when to stop active hours)? (0-23)");
          inString = "";
          inString = Serial.readStringUntil('\n');
          if(inString.toInt() < 24 && inString.toInt() >= 0 && inString.toInt() > settings.active_lowerbound){
            settings.active_upperbound = inString.toInt();
          } else {
            Serial.println("The number you entered was out of range (0-23) or is less than lowerbound.");
          }
        }
        if(settings.active_hours_enable){Serial.printf("  New active hours: {ENABLE: true, START: %d, STOP: %d}\n",settings.active_lowerbound,settings.active_upperbound);}else{Serial.printf("  New active hours: {ENABLE: false}\n");}
      } else if (inString == "exit"){
        break;
      } else {
        Serial.println("Please check command spelling and capitalization.");
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

  for(int i = 0; i < 30; i++){
    delay(1000);
    if(WiFi.status() == WL_CONNECTED){
      Serial.print("WiFi Connection Established! (IP address: ");Serial.print(WiFi.localIP());Serial.println(")");
      timeClient.begin();
      break;
    } else {
      Serial.printf("Attempting WiFi connection (%d)\n",i+1);
    }
  }
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("WiFi Connection Error! (check SSID or Password)");
  }

  syncRTC();
  clearDisplay();
  delay(1000);

}

void clearDisplay(){
  Serial.println("Clearing display...");
  for(int i = 0; i < 12; i++){
    writeDot(i,0);
    delay(settings.flip_delay);
  }
  shownDisplay = 0;
  Serial.println("Display cleared.");
  delay(250);
}

void selfTestTime(){
  Serial.println("Begining self-test-time");
  for(int r = 0; r < 2; r++){
    for(int h = 1; h < 24; h++){
      for(int m = 0; m < 60; m++){
        writeTime(h, m);
        delay(500);
      }
    }
  }
}

void selfTestIndividual(){
  Serial.println("Begining self-test-individual");
  for(int i = 0; i < 2; i++){
    for(int j = 0; j < 12; j++){
      writeDot(j, i);
      delay(1000);
    }
  }
}

void flashDisplay(){
  SR.enable();
  delay(FLASHTIME);
  SR.disable();
}

void writeTime(short hours, short minutes){
  //Serial.printf("IN: H:%d, M:%d\n",hours,minutes);
  short newDisplay = 0;
  short tenMins = (minutes/10)%10;
  short oneMins = minutes%10;
  if(hours/12){ //am/pm indicator
      newDisplay |= 1UL << 4;
  } //else do nothing because it is already 0.
  if(hours == 0){
    hours = 12;
  } else if(hours > 12){
      hours = hours - 12; // for 12h format
  } 
  if(settings.left_is_MSD){
    for(int i = 3; i >= 0; i--){
      newDisplay |= (((hours >> i) & 1UL) << 11-i);
    }
    for(int i = 3; i >= 0; i--){
      newDisplay |= (((oneMins >> i) & 1UL) << 3-i);
    }
  } else {
    for(int i = 3; i >= 0; i--){
      newDisplay |= (((hours >> i) & 1UL) << 3-i);
    }
    for(int i = 3; i >= 0; i--){
      newDisplay |= (((oneMins >> i) & 1UL) << 11-i);
    }
  }
  for(int i = 2; i >= 0; i--){
      newDisplay |= (((tenMins >> i) & 1UL) << 7-i);
  }
  //for(int i = 11; i >= 0; i--){Serial.printf("%d",(newDisplay >> i) & 1UL);} Serial.println();
  for(int i = 11; i >= 0; i--){
      if(((shownDisplay >> i) & 1UL) != ((newDisplay >> i) & 1UL)){
        writeDot(i, ((newDisplay >> i) & 1UL));
        //Serial.printf("%d:%d, ",i,(shownDisplay >> i) & 1UL); Serial.println();
        delay(settings.flip_delay);
      }
  }
  shownDisplay = newDisplay;
  return;
}

void writeDot(char location, bool state){ // one dot at a time
      SR.disable(); // why not
      SR.clear();
      for(short i = 0; i < 12; i++){
        if(i == location && state){
          digitalWrite(SR.serial, LOW);
          SR.incrementSR();
          SR.incrementR();
          digitalWrite(SR.serial, HIGH);
          SR.incrementSR();
          SR.incrementR();
        } else if(i == location && !state){
          digitalWrite(SR.serial, HIGH);
          SR.incrementSR();
          SR.incrementR();
          digitalWrite(SR.serial, LOW);
          SR.incrementSR();
          SR.incrementR();
        } else {
          digitalWrite(SR.serial, LOW);
          SR.incrementSR();
          SR.incrementR();
          SR.incrementSR();
          SR.incrementR();
        }
      }
      flashDisplay();
      return;
}

void syncRTC(){
  if(WiFi.status() == WL_CONNECTED){
    timeClient.update();
    RTC.adjust(DateTime(timeClient.getEpochTime()));    //update RTC
    Serial.println("Synced RTC!");
  } else if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi is NOT connected, could NOT sync time!");
  }
}

void loop() {
  
  while(true){  //wait till next minute
    RTCnow = RTC.now();
    if(RTCnow.minute() != shownMinute){    //update clock
      if(RTCnow.hour() == 12 && RTCnow.minute() == 0){
        syncRTC();
        clearDisplay();
      } 
      if(settings.active_hours_enable){
        if(RTCnow.hour() >= settings.active_lowerbound && RTCnow.hour() < settings.active_upperbound){
          Serial.printf("TIME: %d:%d\n",RTCnow.hour(), RTCnow.minute());
          writeTime(RTCnow.hour(), RTCnow.minute());
        } else {
          Serial.println("Outside active hours.");
          if(shownDisplay != 0){clearDisplay();}
        }
      } else {
        Serial.printf("TIME: %d:%d\n",RTCnow.hour(), RTCnow.minute());
        writeTime(RTCnow.hour(), RTCnow.minute());
      }
      shownMinute = RTCnow.minute();
      break;
    }
    delay(500);
  }   
  delay(30000);
}