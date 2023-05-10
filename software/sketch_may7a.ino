#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

int led = 2;
const long utcOffsetInSeconds = -14400;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void setup() {
  pinMode (led, OUTPUT);
  WiFi.hostname("Flipdot_Clock");
  Serial.begin(9600); 
  Serial.setTimeout(20000);
  Serial.println("\nWelcome to the Flipdot Clock!");
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

void loop() {
  if(WiFi.status() == WL_CONNECTED){
      timeClient.update();
      Serial.print(timeClient.getHours());Serial.print(":");Serial.print(timeClient.getMinutes());Serial.print(":");Serial.println(timeClient.getSeconds());
    }
  digitalWrite(led, HIGH);
  delay(1000);
  digitalWrite(led, LOW);
  delay(1000);
}