#ifndef EXPANSION_H
#define EXPANSION_H

#include <Arduino.h>

#define EXPANSION_NUM_PINS 8

#define EXPANSION_PIN_0 35
#define EXPANSION_PIN_1 36
#define EXPANSION_PIN_2 37
#define EXPANSION_PIN_3 38
#define EXPANSION_PIN_4 39
#define EXPANSION_PIN_5 40
#define EXPANSION_PIN_6 41
#define EXPANSION_PIN_7 42

#define EXPANSION_ADC_RESOLUTION 12

void expansion_init();
bool expansion_digital_read(uint8_t index);
uint16_t expansion_analog_read(uint8_t index);

#endif
