#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

#define FLASHTIME 2000   //in milliseconds


const long utcOffsetInSeconds = -14400;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
/*
struct shiftRegister{
  short serial = 8;
  short OE = 7;
  short RCLK = 6;
  short SRCLK = 5;
  short SRCLR = 3;
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
*/
void setup() {
  /*
  pinMode(SR.serial, OUTPUT);
  pinMode(SR.OE, OUTPUT);
  pinMode(SR.RCLK, OUTPUT);
  pinMode(SR.SRCLK, OUTPUT);
  pinMode(SR.SRCLR, OUTPUT);
  SR.disable();
  SR.clear();
  */
  Serial.begin(9600); 
  Serial.setTimeout(20000);
  WiFi.hostname("Flipdot_Clock");
  Serial.println("\nWelcome to the Flipdot Clock Software!");
  delay(1000);

  struct { 
    char ssid[32] = "";
    char password[64] = "";
  } wifiSettings;
  EEPROM.begin(sizeof(wifiSettings));
  EEPROM.get(0,wifiSettings);

  String newSSID, newPASSWORD;
  Serial.printf("Current WiFi settings: {SSID: %s, PASSWORD: %s}\n",wifiSettings.ssid, wifiSettings.password);
  Serial.println("Enter new WIFI ssid:");
  newSSID = Serial.readStringUntil('\n');
  if(newSSID.length() >= 2){
    strcpy(wifiSettings.ssid, newSSID.c_str());
    Serial.println("Enter new WIFI password:");
    newPASSWORD = Serial.readStringUntil('\n');
    if(newPASSWORD.length() >= 2){
      strcpy(wifiSettings.password, newPASSWORD.c_str());
    }
    Serial.printf("New WiFi settings: {SSID: %s, PASSWORD: %s}\n",wifiSettings.ssid, wifiSettings.password);
    EEPROM.put(0,wifiSettings);
    EEPROM.commit();
  }
  Serial.println("Setup Finished!");
  WiFi.begin(wifiSettings.ssid, wifiSettings.password);             

  
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
  //make something for timezone offsets? 
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