#ifndef ENVELOPE_CONTROL_H
#define ENVELOPE_CONTROL_H

#include <Arduino.h>
#include "servo_config.h"

typedef struct {
  bool     enabled;
  uint8_t  servo_ch;
  uint16_t min_pulse;
  uint16_t max_pulse;
  bool     invert;
  uint16_t gain;
  uint8_t  smoothing;
  uint16_t interval_ms;
} envelope_ctrl_cfg_t;

void         envelope_ctrl_init();
void         envelope_ctrl_load();
void         envelope_ctrl_save();
envelope_ctrl_cfg_t *envelope_ctrl_get();
void         envelope_ctrl_apply();
bool         envelope_ctrl_active();

#endif
