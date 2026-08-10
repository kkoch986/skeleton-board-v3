#include "envelope_detect.h"
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ENVELOPE_SAMPLE_PERIOD_MS 1
#define ENVELOPE_WINDOW_MS 10

static uint16_t dc_offset = ENVELOPE_ADC_MAX / 2;
static uint16_t avg_buffer[ENVELOPE_AVG_FRAMES];
static uint8_t avg_index = 0;
static uint32_t avg_sum = 0;
static uint16_t gain = 128;
static uint8_t smoothing = 0;
static uint16_t smoothed = 0;
static bool has_sample = false;
static volatile uint16_t latest_raw = 0;
static volatile uint16_t latest_amp = 0;
static bool task_started = false;

static void envelope_task(void *arg) {
  uint16_t win_peak = 0;
  uint32_t win_start_ms = millis();

  for (;;) {
    uint16_t raw = analogRead(ENVELOPE_ADC_PIN);

    avg_sum -= avg_buffer[avg_index];
    avg_buffer[avg_index] = raw;
    avg_sum += raw;
    avg_index = (avg_index + 1) % ENVELOPE_AVG_FRAMES;
    uint16_t averaged = avg_sum / ENVELOPE_AVG_FRAMES;
    int32_t deviation = (int32_t)averaged - dc_offset;
    uint16_t amp = deviation < 0 ? -deviation : deviation;
    if (amp > win_peak) win_peak = amp;

    if (millis() - win_start_ms >= ENVELOPE_WINDOW_MS) {
      latest_raw = map(win_peak, 0, dc_offset, 0, 255);

      uint16_t scaled = (uint32_t)latest_raw * gain / 64;
      if (scaled > 255) scaled = 255;

      if (!has_sample) {
        smoothed = scaled;
        has_sample = true;
      } else if (smoothing == 0) {
        smoothed = scaled;
      } else {
        smoothed = ((uint32_t)smoothed * (256 - smoothing) + (uint32_t)scaled * smoothing) / 256;
      }
      latest_amp = smoothed;

      win_peak = 0;
      win_start_ms = millis();
    }

    vTaskDelay(ENVELOPE_SAMPLE_PERIOD_MS);
  }
}

void envelope_detect_init() {
  pinMode(ENVELOPE_DETECT_PIN, INPUT);
  analogReadResolution(ENVELOPE_ADC_RESOLUTION);

  uint32_t sum = 0;
  for (int i = 0; i < ENVELOPE_CALIBRATE_SAMPLES; i++) {
    sum += analogRead(ENVELOPE_ADC_PIN);
    delay(2);
  }
  dc_offset = sum / ENVELOPE_CALIBRATE_SAMPLES;

  for (int i = 0; i < ENVELOPE_AVG_FRAMES; i++) {
    avg_buffer[i] = 0;
  }
  avg_sum = 0;
  avg_index = 0;
  has_sample = false;
  latest_raw = 0;
  latest_amp = 0;

  if (!task_started) {
    task_started = true;
    xTaskCreatePinnedToCore(envelope_task, "env", 4096, NULL, 5, NULL, 0);
  }
}

bool envelope_detect_connected() {
  return digitalRead(ENVELOPE_DETECT_PIN) == LOW;
}

uint16_t envelope_detect_amplitude() {
  return latest_amp;
}

uint16_t envelope_detect_amplitude_raw() {
  return latest_raw;
}

void envelope_detect_set_gain(uint16_t g) {
  gain = g;
}

uint16_t envelope_detect_get_gain() {
  return gain;
}

void envelope_detect_set_smoothing(uint8_t s) {
  smoothing = s;
}

uint8_t envelope_detect_get_smoothing() {
  return smoothing;
}
