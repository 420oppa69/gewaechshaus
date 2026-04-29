// Libs und creds
#include "creds.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <AccelStepper.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 10 // S3
#define DHTTYPE DHT22 // AM2302
#define StepperEnable 2 // D4
#define BeleuchtungsPin 0 // D3
#define IN1 14 // D5
#define IN2 12 // D6
#define IN3 13 // D7
#define IN4 15 // D8
#define Endlage 9 // S2
bool linksLauf = false;
bool rechtsLauf = false;
int lcdColumns = 16;
int lcdRows = 2;
// Instanzen
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN2, IN3, IN4);
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
unsigned long lastMsg; // Zeitstempel für millis() später
const int ldr = A0; // Analoger Input des Light Dependent Resistors (Photowiderstand), mehr Licht = höherer analoger Wert

void setup() {
  // Serielle Kommunikation initialisieren, extra bisschen delay damit erster Print auch sichtbar ist nachdem der ESP erst Müll ausspuckt
  Serial.begin(115200);
  pinMode(BeleuchtungsPin, OUTPUT);
  digitalWrite(BeleuchtungsPin, LOW);
  pinMode(StepperEnable, OUTPUT);
  digitalWrite(StepperEnable, LOW);
  pinMode(Endlage, INPUT);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Verbinde WLAN...");
  delay(2000);
  Serial.println("");
  Serial.println("Verbinde mit Wlan...");
  //Onboardled deklarieren 
  int onboardLed = 2;
  bool onboardLedStatus = false;
  pinMode(onboardLed, OUTPUT);
  // Mit Wlan verbinden, siehe creds.h (auf github versteckt natürlich hihi)
  WiFi.begin(ssid, password);
  // Wartet bis Wlan verbunden ist und blinkt onboardled
  while(WiFi.status() != WL_CONNECTED) {
    digitalWrite(onboardLed, onboardLedStatus);
    onboardLedStatus = !onboardLedStatus;
    delay(500);
    Serial.print(".");
  }
  Serial.println("Mit Wlan verbunden, IP: ");
  Serial.print(WiFi.localIP());
  onboardLedStatus = true;          // Led iwie negiert
  digitalWrite(onboardLed, onboardLedStatus);
  // MQTT Server Details aufsetzen
  client.setServer(mqtt_server, 1883);
  // DHT Begin und sowas
  dht.begin();
  lcd.setCursor(0, 1);
  lcd.print("WLAN verbunden!");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Checkt ob MQTT Broker verbunden ist
  if(!client.connected()) {
    reconnect();
  }
  // Zeitabstand zwischen gesendeten Nachrichten
  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    // Payload Helligkeit
    unsigned int ldrHelligkeit;
    ldrHelligkeit = analogRead(A0);
    Serial.print("Helligkeit: ");
    Serial.println(ldrHelligkeit);
    // Konvertieren vom Integer vom Payload in ein Character Array, kann auch länger sein
    char msg_out[8];
    sprintf(msg_out, "%d", ldrHelligkeit);
    // Und sendet hier Nachricht an Broker
    client.publish("gewaechshaus/helligkeit", msg_out);
    lcd.clear();
    // Payload Temperatur und Luftfeuchtigkeit
    float temperatur = 0.0;
    float luftfeuchtigkeit = 0.0;
    float newT = dht.readTemperature();
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT");
    } else {
      temperatur = newT;
      Serial.print("Temperatur: ");
      Serial.println(temperatur);
      // Auch hier konvertieren, nur halt von einem Float
      char msg_out[4];
      sprintf(msg_out, "%f", temperatur);
      // Publish für Temperatur
      client.publish("gewaechshaus/temperatur", msg_out);
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "Tmp: %.2f", temperatur);
      lcd.setCursor(0, 0);          // Auf LCD printen
      lcd.print(buffer);
    } // Und das gleiche für die Luftfeuchtigkeit
    float newH = dht.readHumidity();
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT");
    } else {
      luftfeuchtigkeit = newH;
      Serial.print("Luftfeuchtigkeit: ");
      Serial.println(luftfeuchtigkeit);
      // Konvertierung
      char msg_out[4];
      sprintf(msg_out, "%f", luftfeuchtigkeit);
      client.publish("gewaechshaus/luftfeuchtigkeit", msg_out);
      lcd.setCursor(0, 1);          // Auf LCD printen
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "Hum: %.2f", luftfeuchtigkeit);
      lcd.print(buffer);
    }
    bool endlagenBool = false;
    endlagenBool = digitalRead(Endlage);
    if (endlagenBool == true) {
      Serial.print("Endlage: ");
      Serial.println(endlagenBool);
      // Konvertierung
      char msg_out[1];
      sprintf(msg_out, "%b", endlagenBool);
      client.publish("gewaechshaus/endlage", msg_out);
    } else {
      Serial.print("Endlage: ");
      Serial.println(endlagenBool);
      sprintf(msg_out, "%b", endlagenBool);
      client.publish("gewaechshaus/endlage", msg_out);
    }
  }

}

// Noch abwandeln, Wlan-Abfrage hinzufügen
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


















