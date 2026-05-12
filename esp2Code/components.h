#ifndef components_h
#define components_h
#include "Arduino.h"

//Klasse zum Ansteuern einer einzelnen LED am definierten Ausgang - Ein/Ausschalten und Blinken
class LED {
  private:
      uint8_t _pin;
      uint8_t _mode;  //0-Aus, 1-An, 2-Blinken
      bool zustand;
      uint32_t aktMillis;
      uint32_t intervallMillis = 1000;
  
  public:
    LED(uint8_t _pin);                        //Konstruktor, Auswahl des Pins
    void update();                            //Aufruf im Loop
    void setMode(uint8_t mode);               //Wechseln der Mode (0 -> Aus, 1 -> Ein, 2 -> Blinken)
    void setInterval(uint32_t _intervall);    //Einstellen des Blinkintervalls
    
};

//Klasse zu Auslesen eines Seeed-Grove-Wasserstandssensor mit I2C-Schnittstelle
class waterLevelSensor{
  private:
    unsigned char _low_data[8] = {0};
    unsigned char _high_data[12] = {0};
    uint8_t _threshold = 100;
    void _getHigh12SectionValue(void);
    void _getLow8SectionValue(void);

  public:
    waterLevelSensor(uint8_t threshold);    //Konstruktor, Einstellen des Grenzwertes zur Fluessigkeitserkennung
    uint8_t read();                         //Sensorwert auslesen
  };

//Klasse zum Darstellen eines Fuellstandes auf einer Reihe von LEDs
class levelDisplay{
  private:
    uint8_t _level; 
    uint8_t _pins[3]; //Unterste zuerst
    uint32_t aktMillis;
    uint32_t interval = 1000;
    bool led1state=LOW;

  public:
    levelDisplay(uint8_t pin1,uint8_t pin2,uint8_t pin3);   //Konstruktor, Auswahl der Pins in aufsteigender Reihenfolge
    void setLevel(uint8_t level);                           //Wasserstand einstellen (0-> Blinken; 1-> unterste LED leuchtet; ... ; 3 -> Alle LEDs leuchten)
    void update();                                          //Aufruf im Loop
};

//Klasse zum Auslesen eines analogen Bodenfeuchtesensors
class soilHumSensor{
  private:
    uint8_t _pin;

  public:
    soilHumSensor(uint8_t pin);                             //Konstruktor, Auswahl des analogen Eingangs
    uint16_t read();                                        //Sensorwert auslesen
};

//Klasse zum Ansteuern einer Wasserpumpe fuer bestimmte Zeitintervalle
class waterPump{
  private:
    uint8_t _pin;
    bool _enabled;
    bool _active;
    uint32_t _startTime;
    uint16_t _durationSetpoint;

  public:
    waterPump(uint8_t pin);                               //Konstruktor, Auswahl des Ausgangs-Pin
    void start(uint16_t duration);                        //Pumpe fuer die angegebene Anzahl an ms starten
    void handle();                                        //Aufruf im Loop
    void enable();                                        //Pumpe freischalten 
    void disable();                                       //Pumpe sperren (z.B. bei zu geringem Wasserstand)
};
#endif