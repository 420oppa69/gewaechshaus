#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include "creds.h"
#include "components.h"

#define levelCheckInterval 250
#define publishInterval 5000
#define minWaterLevel 15
#define minSoilHum 250

uint32_t lastLevelCheckTime = 0;
uint32_t lastPublishTime = 0;
uint16_t soilHumRead = 0;
uint8_t waterLevel = 0;
uint8_t waterDisplayLevel = 0;


WiFiClient espClient;
PubSubClient client(espClient);
LED debugLED(D4);
waterLevelSensor waterLevelSensor(100);
levelDisplay waterLevelDisplay(D5,D6,D7,D8,10);
soilHumSensor soilHumSensor(A0);
waterPump waterPump(D0);

void setup() {
  Serial.begin(115200);
  delay(2000);

  debugLED.setMode(2);
  debugLED.setInterval(250);

  Serial.println("Verbinde mit Wlan...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    debugLED.update();
  }

  debugLED.setMode(1);
  Serial.print("Mit Wlan verbunden! IP Addresse: ");
  Serial.println(WiFi.localIP());
  
  client.setServer(mqtt_server, 1883);
  init_OTA();
  Wire.begin();

}

void loop() {
  if(!client.connected()) {
    reconnect();
  }
  ArduinoOTA.handle();
  
  debugLED.setMode(2);
  debugLED.setInterval(1000);
  
  //Check Waterlevel
  if (millis()-lastLevelCheckTime > levelCheckInterval){
    waterLevel = waterLevelSensor.read();
    if (waterLevel < minWaterLevel){
        waterDisplayLevel = 0;
        waterPump.disable();
      }
    else{
        waterDisplayLevel = map(waterLevel, minWaterLevel, 100, 1,5);
        waterPump.enable();
    }
    waterLevelDisplay.setLevel(waterDisplayLevel);
  }

  //Publish Data
  if (millis()-lastPublishTime > publishInterval){
    soilHumRead = soilHumSensor.read();
    if (soilHumRead < minSoilHum){
      waterPump.start(5000);
    }

    char msg1_out[8];
    sprintf(msg1_out, "%d", waterLevel);
    client.publish("gewaechshaus/Fuellstand_Wassertank", msg1_out);
    
    char msg2_out[8];
    sprintf(msg2_out, "%d", soilHumRead);
    // Und sendet hier Nachricht an Broker
    client.publish("gewaechshaus/bodenfeuchte", msg2_out);
  }
  
  waterPump.handle();  
  debugLED.update();
  waterLevelDisplay.update();
}


void reconnect() {
  while(!client.connected()) {
    Serial.println("");
    Serial.print("MQTT nicht verbunden, versuche zu verbinden... ");
    // Macht irgendeine Client-ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Versuche zu verbinden
    if(client.connect(clientId.c_str(), mqtt_user, mqtt_pwd)) {
      Serial.println("Verbunden.");
    } else {
      Serial.print("Fehler, code=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void init_OTA(){
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();
}