#ifndef ENVELOPE_DETECT_H
#define ENVELOPE_DETECT_H

#include <Arduino.h>

#define ENVELOPE_ADC_PIN 1
#define ENVELOPE_DETECT_PIN 2

#define ENVELOPE_ADC_RESOLUTION 12
#define ENVELOPE_ADC_MAX 4095

#define ENVELOPE_CALIBRATE_SAMPLES 256
#define ENVELOPE_AVG_FRAMES 4

void envelope_detect_init();
bool envelope_detect_connected();
uint16_t envelope_detect_amplitude();

#endif
