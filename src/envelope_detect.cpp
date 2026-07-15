#include "envelope_detect.h"

static uint16_t dc_offset = ENVELOPE_ADC_MAX / 2;
static uint16_t avg_buffer[ENVELOPE_AVG_FRAMES];
static uint8_t avg_index = 0;
static uint32_t avg_sum = 0;

static uint16_t adc_read_raw() {
  return analogRead(ENVELOPE_ADC_PIN);
}

void envelope_detect_init() {
  pinMode(ENVELOPE_DETECT_PIN, INPUT);
  analogReadResolution(ENVELOPE_ADC_RESOLUTION);

  uint32_t sum = 0;
  for (int i = 0; i < ENVELOPE_CALIBRATE_SAMPLES; i++) {
    sum += adc_read_raw();
    delay(2);
  }
  dc_offset = sum / ENVELOPE_CALIBRATE_SAMPLES;

  for (int i = 0; i < ENVELOPE_AVG_FRAMES; i++) {
    avg_buffer[i] = 0;
  }
  avg_sum = 0;
  avg_index = 0;
}

bool envelope_detect_connected() {
  return digitalRead(ENVELOPE_DETECT_PIN) == LOW;
}

uint16_t envelope_detect_amplitude() {
  uint16_t raw = adc_read_raw();

  avg_sum -= avg_buffer[avg_index];
  avg_buffer[avg_index] = raw;
  avg_sum += raw;
  avg_index = (avg_index + 1) % ENVELOPE_AVG_FRAMES;

  uint16_t averaged = avg_sum / ENVELOPE_AVG_FRAMES;
  int32_t deviation = (int32_t)averaged - dc_offset;
  uint16_t amplitude = deviation < 0 ? -deviation : deviation;
  return map(amplitude, 0, dc_offset, 0, 255);
}
