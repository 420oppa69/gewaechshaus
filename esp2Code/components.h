#ifndef components_h
#define components_h
#include "Arduino.h"

class LED {
  public:
    LED(uint8_t _pin);
    void update();
    void setMode(uint8_t mode); 
    void setInterval(uint32_t _intervall);
    
  private:
      uint8_t _pin;
      uint8_t _mode;  //0-Aus, 1-An, 2-Blinken
      bool zustand;
      uint32_t aktMillis;
      uint32_t intervallMillis = 1000;

};

class waterLevelSensor{
  private:
    unsigned char _low_data[8] = {0};
    unsigned char _high_data[12] = {0};
    uint8_t _threshold = 100;
    void _getHigh12SectionValue(void);
    void _getLow8SectionValue(void);

  public:
    waterLevelSensor(uint8_t threshold);
    uint8_t read();
  };


class levelDisplay{
  private:
    uint8_t _level; 
    uint8_t _pins[5]; //Unterste zuerst
    uint32_t aktMillis;
    uint32_t interval = 1000;
    bool led1state=LOW;

  public:
    levelDisplay(uint8_t pin1,uint8_t pin2,uint8_t pin3,uint8_t pin4,uint8_t pin5 );
    void setLevel(uint8_t level);
    void update();
};


class soilHumSensor{
  private:
    uint8_t _pin;

  public:
    soilHumSensor(uint8_t pin);
    uint16_t read();
};


class waterPump{
  private:
    uint8_t _pin;
    bool _enabled;
    bool _active;
    uint32_t _startTime;
    uint16_t _durationSetpoint;

  public:
    waterPump(uint8_t pin);
    void start(uint16_t duration);
    void handle();
    void enable();
    void disable();
};
#endif