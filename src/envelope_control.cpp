#include "envelope_control.h"
#include <Preferences.h>
#include "envelope_detect.h"
#include "servo_config.h"
#include "servo_pca9685.h"

static Preferences prefs;
static envelope_ctrl_cfg_t cfg;
static uint32_t last_apply_ms = 0;

static void defaults() {
  cfg.enabled   = false;
  cfg.servo_ch  = 0;
  cfg.min_pulse = SERVO_MIN_PULSE;
  cfg.max_pulse = SERVO_MAX_PULSE;
  cfg.invert    = false;
  cfg.gain      = 128;
  cfg.smoothing = 40;
  cfg.interval_ms = 20;
}

void envelope_ctrl_init() {
  defaults();
  envelope_ctrl_load();
  servo_config_set_ramp_exempt(cfg.enabled ? (int8_t)cfg.servo_ch : -1);
}

void envelope_ctrl_load() {
  prefs.begin("env_ctrl", true);
  if (prefs.isKey("cfg2")) {
    prefs.getBytes("cfg2", &cfg, sizeof(envelope_ctrl_cfg_t));
  }
  prefs.end();
  envelope_detect_set_gain(cfg.gain);
  envelope_detect_set_smoothing(cfg.smoothing);
}

void envelope_ctrl_save() {
  prefs.begin("env_ctrl", false);
  prefs.putBytes("cfg2", &cfg, sizeof(envelope_ctrl_cfg_t));
  prefs.end();
  servo_config_set_ramp_exempt(cfg.enabled ? (int8_t)cfg.servo_ch : -1);
}

envelope_ctrl_cfg_t *envelope_ctrl_get() {
  return &cfg;
}

bool envelope_ctrl_active() {
  return cfg.enabled && envelope_detect_connected();
}

void envelope_ctrl_apply() {
  if (!envelope_ctrl_active()) return;
  if (millis() - last_apply_ms < cfg.interval_ms) return;
  last_apply_ms = millis();

  servo_cfg_t *sv = servo_config_get(cfg.servo_ch);
  if (!sv || !sv->enabled) return;

  servo_state_t *st = servo_state_get(cfg.servo_ch);
  if (!st || st->manual) return;

  uint16_t amplitude = envelope_detect_amplitude();

  uint16_t pulse;
  if (cfg.invert) {
    pulse = map(amplitude, 0, 255, cfg.max_pulse, cfg.min_pulse);
  } else {
    pulse = map(amplitude, 0, 255, cfg.min_pulse, cfg.max_pulse);
  }

  if (pulse < cfg.min_pulse) pulse = cfg.min_pulse;
  if (pulse > cfg.max_pulse) pulse = cfg.max_pulse;

  servo_config_set_target(cfg.servo_ch, pulse);
}
