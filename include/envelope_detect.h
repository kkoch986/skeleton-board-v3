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
uint16_t envelope_detect_amplitude_raw();
void envelope_detect_set_gain(uint16_t gain);
uint16_t envelope_detect_get_gain();
void envelope_detect_set_smoothing(uint8_t smoothing);
uint8_t envelope_detect_get_smoothing();

#endif
