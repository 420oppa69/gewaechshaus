/*
Gewaechshaus - Bewaesserungssteuerung v0.9
Liest Bodenfeuchte- und Fuellstandssensor ein
Steuert LED-Fuellstandsanzeige
Bei zu geringer Bodenfeuchte und ausreichendem Wasserstand wird bewaessert
Sensorwerte werden per MQTT geteilt

created by:
Henri Geuer
BBS Brinkstraße - EPV41 - LF7
Herr Kahmann

29.04.2026
*/

//Bibliotheken
#include <ESP8266WiFi.h>  //WLAN
#include <WiFiUdp.h>      
#include <PubSubClient.h> //MQTT
#include <ArduinoOTA.h>   //Other the Air updates
#include <Wire.h>         //I2C Kommunikation
#include "creds.h"        //Anmeldedaten für WLAN und MQTT-Broker
#include "components.h"   //Eigene Bibliothek mit Klassen für alle Komponenten

//Konfiguration
#define levelCheckInterval 250   //Intervall zur Fuellstandsueberwachung des Wasserstandes
#define publishInterval 5000    //Intervall zur Messung und veröffentlichung der Messwerte 
#define minWaterLevel 15        //Wasserstand in %, bei dem die Pumpe deaktiviert wird
#define maxSoilHum 1020          //Analogwert des Bodenfeuchtesensors, bei dessen Ueberschreitung ein Bewaesserungsvorgang ausgelöst wird


//Variablendeklaration und Definition
uint32_t lastLevelCheckTime = 0;   //Zeitstempel fuer Wasserstandsueberwachung
uint32_t lastPublishTime = 0;      //Zeitstempel fuer Messwertveroeffentlichung
uint16_t soilHumRead = 0;         //Analogwert des Bodenfeuchtesensors
uint8_t waterLevel = 0;           //Wasserstand in %
uint8_t waterDisplayLevel = 0;    //Hoehe der Fuellstandsanzeige (0 -> Blinkend; 1-> erste LED leuchtet; ... ; 3 -> Alle LEDs leuchten )
bool pumpStarted = false;

//Instanzen
WiFiClient espClient;                       //WLAN-Verbindung
PubSubClient client(espClient);             //MQTT-Client
LED debugLED(D8);                           //Debug - LED auf Platine
waterLevelSensor waterLevelSensor(100);     //Fuellstandssensor
levelDisplay waterLevelDisplay(10,12,13);   //Wasserstandsanzeige aus 3 LEDs
soilHumSensor soilHumSensor(A0);            //Bodenfeuchtesensor
waterPump waterPump(D0);                    //Wasserpumpe


void setup() {
  Serial.begin(115200);
  delay(2000);

  //WLAN Verbindung aufbauen
  Serial.println("Verbinde mit Wlan...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    debugLED.update();
  }
  //OTA einrichten
  ArduinoOTA.setHostname("ESP8266_bewaesserung");
  ArduinoOTA.setPassword("OTA");
  ArduinoOTA.begin();

  Serial.print("Mit Wlan verbunden! IP Addresse: ");
  Serial.println(WiFi.localIP());
  
  //MQTT-Client einrichten
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //Kommunikation mit Fuellstandssensor beginnen
  Wire.begin();
}

void loop() {
  //Verbindung mit MQTT-Server aufbauen
  if(!client.connected()) {
    connect();
  }
  //Normalbetrieb: LED blinkt mit 1 Hz
  debugLED.setMode(2);
  debugLED.setInterval(1000);

  //Wasserstand ueberpruefen
  if (millis()-lastLevelCheckTime > levelCheckInterval){
    waterLevel = waterLevelSensor.read();
    //Wasserstand zu niedrig -> Pumpe deaktivieren
    if (waterLevel < minWaterLevel){
        waterDisplayLevel = 0;
        waterPump.disable();
      }
    //Wasserstand ok -> Normalbetrieb
    else{
        waterDisplayLevel = map(waterLevel, minWaterLevel, 100, 1,5);
        waterPump.enable();
    }
    waterLevelDisplay.setLevel(waterDisplayLevel);
  }

  //MQTT-Steuerrungssignal zuruecksetzen, wenn Pumpe gestartet wurde
  if (pumpStarted== true){
    client.publish("gewaechshaus/ctrl/pump", "0");
    pumpStarted = false;
  }

  //Sensoren auslesen und Werte veroeffentlichen
  if (millis()-lastPublishTime > publishInterval){
    soilHumRead = soilHumSensor.read();
    if (soilHumRead > maxSoilHum){
      waterPump.start(1000);
    }

    //Payload fuer Fuellstand erstellen
    char msg1_out[8];
    sprintf(msg1_out, "%d", waterLevel);
    client.publish("gewaechshaus/Fuellstand_Wassertank", msg1_out);
    Serial.print("Fuellstand: ");
    Serial.println(waterLevel);

    //Payload fuer Bodenfeuchte erstellen
    char msg2_out[8];
    sprintf(msg2_out, "%d", soilHumRead);
    // Und sendet hier Nachricht an Broker
    client.publish("gewaechshaus/bodenfeuchte", msg2_out);
    Serial.print("Bodenfeuchte: ");
    Serial.println(soilHumRead);
  }
  
  //Handler fuer Instanzen
  waterPump.handle();  
  debugLED.update();
  waterLevelDisplay.update();
  client.loop();
  ArduinoOTA.handle();
}

//Verbindung mit MQTT-Server aufbauen
void connect() {
  while(!client.connected()) {
    Serial.println("");
    Serial.print("MQTT nicht verbunden, versuche zu verbinden... ");
    // Macht irgendeine Client-ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Versuche zu verbinden
    if(client.connect(clientId.c_str(), mqtt_user, mqtt_pwd)) {
      Serial.println("Verbunden.");
      client.subscribe("gewaechshaus/ctrl/pump");
    } else {
      Serial.print("Fehler, code=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

//Wird aufgerufen, wenn Steuerungssignal fuer Wasserpumpe per MQTT empfangen wird
void callback(char* topic, byte* payload, unsigned int length) {
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    Serial.println("Wasserpumpe von remote gestartet");
    pumpStarted = true;
    waterPump.start(2000);
  }
}