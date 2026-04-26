#include "arduino.h"
#include "components.h"
#include <Wire.h>

#define ATTINY1_HIGH_ADDR 0x78
#define ATTINY2_LOW_ADDR 0x77

//LED------------------------------------------------------------------------------------------------------
LED::LED(uint8_t pin) {
  _pin = pin;
  zustand = LOW;
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, zustand);
}


void LED::update() {
  if (_mode == 0) {
    zustand = LOW;
  } else if (_mode == 1) {
    zustand = HIGH;
  } else if (_mode == 2) {
    if (millis() - aktMillis >= intervallMillis) {
      aktMillis = millis();
      zustand = !zustand;
    }
  }
  digitalWrite(_pin, zustand);
}

void LED::setMode(uint8_t mode) {
  _mode = mode;
}


void LED::setInterval(uint32_t _intervall) {
  intervallMillis = _intervall;
}

//WaterLevelSensor-----------------------------------------------------------------------------------
waterLevelSensor::waterLevelSensor(uint8_t threshold) {
  _threshold = threshold;
}

void waterLevelSensor::_getHigh12SectionValue(void) {
  memset(_high_data, 0, sizeof(_high_data));
  Wire.requestFrom(ATTINY1_HIGH_ADDR, 12);
  while (12 != Wire.available())
    ;

  for (int i = 0; i < 12; i++) {
    _high_data[i] = Wire.read();
  }
  delay(10);
}

void waterLevelSensor::_getLow8SectionValue(void) {
  memset(_low_data, 0, sizeof(_low_data));
  Wire.requestFrom(ATTINY2_LOW_ADDR, 8);
  while (8 != Wire.available())
    ;

  for (int i = 0; i < 8; i++) {
    _low_data[i] = Wire.read();  // receive a byte as character
  }
  delay(10);
}

uint8_t waterLevelSensor::read() {
  int sensorvalue_min = 250;
  int sensorvalue_max = 255;
  int low_count = 0;
  int high_count = 0;
  uint32_t touch_val = 0;
  uint8_t trig_section = 0;

  _getLow8SectionValue();
  _getHigh12SectionValue();
  for (int i = 0; i < 8; i++) {
    if (_low_data[i] >= sensorvalue_min && _low_data[i] <= sensorvalue_max) {
      low_count++;
    }
  }
  for (int i = 0; i < 12; i++) {
    if (_high_data[i] >= sensorvalue_min && _high_data[i] <= sensorvalue_max) {
      high_count++;
    }
  }

  for (int i = 0; i < 8; i++) {
    if (_low_data[i] > _threshold) {
      touch_val |= 1 << i;
    }
  }
  for (int i = 0; i < 12; i++) {
    if (_high_data[i] > _threshold) {
      touch_val |= (uint32_t)1 << (8 + i);
    }
  }

  while (touch_val & 0x01) {
    trig_section++;
    touch_val >>= 1;
  }
  return trig_section;
}

//LevelDisplay-----------------------------------------------------------------------------------------------
levelDisplay::levelDisplay(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4, uint8_t pin5) {
  _pins[0] = pin1;
  _pins[1] = pin2;
  _pins[2] = pin3;
  _pins[3] = pin4;
  _pins[4] = pin5;

  for (int i = 0; i < 5; i++) {
    pinMode(_pins[i], OUTPUT);
    digitalWrite(_pins[i], LOW);
  }
}

void levelDisplay::setLevel(uint8_t level) {
  _level = level;
}

void levelDisplay::update() {
  if (_level == 0) {
    for (int i = 1; i < 5; i++) {
      digitalWrite(_pins[i], LOW);
    }
    if (millis() - aktMillis >= interval) {
      aktMillis = millis();
      led1state = !led1state;
    }
    digitalWrite(_pins[0], led1state);
  } else {
    for (int i = 0; i < 5; i++) {
      if (i < _level) {
        digitalWrite(_pins[i], HIGH);
      } else {
        digitalWrite(_pins[i], LOW);
      }
    }
  }
}

//SoilHumSensor
soilHumSensor::soilHumSensor(uint8_t pin){
  _pin = pin;
  pinMode(_pin, INPUT);
}

uint16_t soilHumSensor::read(){
  return analogRead(_pin);
}

//WaterPump
waterPump::waterPump(uint8_t pin){
  _pin = pin;
  pinMode(_pin,OUTPUT);
  _enabled = false;
}

void waterPump::start(uint16_t duration){
  if (_active == false){
    _active = true;
    _startTime = millis();
    _durationSetpoint = duration;
  }
}
void waterPump::enable(){
  _enabled = true;
}

void waterPump::disable(){
  _enabled =false;
}

void waterPump::handle(){
  if (_active == true){
    if (_enabled == true){
      digitalWrite(_pin, HIGH);
    }
    if (millis()-_startTime > _durationSetpoint){
      _active = false;
    }
  }
  if (_enabled == false || _active == false){
    digitalWrite(_pin, LOW);
  }
}