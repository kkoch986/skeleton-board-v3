#include "servo_config.h"
#include <Arduino.h>
#include <Preferences.h>
#include "servo_pca9685.h"

static Preferences prefs;
static servo_cfg_t  configs[SERVO_NUM_CHANNELS];
static servo_state_t states[SERVO_NUM_CHANNELS];

#define SERVO_FIRST_RAMP_SPEED 150

static bool     first_ramp_active = false;
static uint32_t first_ramp_last_ms = 0;
static int8_t   ramp_exempt_ch = -1;
static uint16_t last_pulse[SERVO_NUM_CHANNELS];

static void defaults(servo_cfg_t *cfg) {
  cfg->lower_limit = SERVO_MIN_PULSE;
  cfg->upper_limit = SERVO_MAX_PULSE;
  cfg->center      = (SERVO_MIN_PULSE + SERVO_MAX_PULSE) / 2;
  cfg->smoothing   = 0;
  cfg->enabled     = false;
  cfg->invert      = false;
  cfg->label[0]    = '\0';
}

static void state_defaults(servo_state_t *st, uint16_t center) {
  st->current = center;
  st->target  = center;
  st->arrived = true;
  st->manual  = false;
}

static void nvs_key(uint8_t ch, char *buf) {
  sprintf(buf, "sv%u", ch);
}

void servo_config_init() {
  for (uint8_t ch = 0; ch < SERVO_NUM_CHANNELS; ch++) {
    defaults(&configs[ch]);
    state_defaults(&states[ch], configs[ch].center);
    last_pulse[ch] = 0xFFFF;
  }
  servo_config_load_all();
}

void servo_config_load(uint8_t ch) {
  if (ch >= SERVO_NUM_CHANNELS) return;
  char key[8];
  nvs_key(ch, key);

  prefs.begin("servo", true);
  if (prefs.isKey(key)) {
    prefs.getBytes(key, &configs[ch], sizeof(servo_cfg_t));
  }
  prefs.end();

  state_defaults(&states[ch], configs[ch].center);
}

void servo_config_save(uint8_t ch) {
  if (ch >= SERVO_NUM_CHANNELS) return;
  char key[8];
  nvs_key(ch, key);

  prefs.begin("servo", false);
  prefs.putBytes(key, &configs[ch], sizeof(servo_cfg_t));
  prefs.end();
}

void servo_config_load_all() {
  for (uint8_t ch = 0; ch < SERVO_NUM_CHANNELS; ch++) {
    servo_config_load(ch);
  }
}

void servo_config_save_all() {
  for (uint8_t ch = 0; ch < SERVO_NUM_CHANNELS; ch++) {
    servo_config_save(ch);
  }
}

servo_cfg_t *servo_config_get(uint8_t ch) {
  if (ch >= SERVO_NUM_CHANNELS) return NULL;
  return &configs[ch];
}

servo_state_t *servo_state_get(uint8_t ch) {
  if (ch >= SERVO_NUM_CHANNELS) return NULL;
  return &states[ch];
}

uint16_t servo_config_apply_limits(uint8_t ch, uint16_t value) {
  if (ch >= SERVO_NUM_CHANNELS) return value;
  servo_cfg_t *cfg = &configs[ch];
  if (value < cfg->lower_limit) value = cfg->lower_limit;
  if (value > cfg->upper_limit) value = cfg->upper_limit;
  return value;
}

void servo_config_set_target(uint8_t ch, uint16_t target) {
  if (ch >= SERVO_NUM_CHANNELS) return;
  servo_cfg_t *cfg = &configs[ch];
  if (!cfg->enabled) return;

  target = servo_config_apply_limits(ch, target);
  states[ch].target  = target;
  states[ch].arrived = false;
}

void servo_config_begin_first_ramp() {
  first_ramp_active = true;
  first_ramp_last_ms = millis();
}

void servo_config_set_ramp_exempt(int8_t ch) {
  ramp_exempt_ch = ch;
}

void servo_config_update() {
  uint32_t now = millis();
  uint32_t dt = now - first_ramp_last_ms;
  first_ramp_last_ms = now;
  uint16_t ramp_step = (SERVO_FIRST_RAMP_SPEED * dt) / 1000;
  if (ramp_step == 0) ramp_step = 1;
  if (ramp_step > 40) ramp_step = 40;

  uint16_t out[SERVO_NUM_CHANNELS];
  bool any_change = false;

  for (uint8_t ch = 0; ch < SERVO_NUM_CHANNELS; ch++) {
    servo_cfg_t   *cfg  = &configs[ch];
    servo_state_t *st   = &states[ch];

    if (!cfg->enabled) {
      out[ch] = st->current;
      if (last_pulse[ch] != st->current) {
        last_pulse[ch] = st->current;
        any_change = true;
      }
      continue;
    }

    if (!st->arrived) {
      if (first_ramp_active && !st->manual && (int8_t)ch != ramp_exempt_ch) {
        if (abs((int)st->current - (int)st->target) <= ramp_step) {
          st->current = st->target;
        } else if (st->current < st->target) {
          st->current += ramp_step;
        } else {
          st->current -= ramp_step;
        }
      } else {
        if (cfg->smoothing == 0 || abs((int)st->current - (int)st->target) <= cfg->smoothing) {
          st->current = st->target;
        } else if (st->current < st->target) {
          st->current += cfg->smoothing;
        } else {
          st->current -= cfg->smoothing;
        }
      }

      if (st->current == st->target) {
        st->arrived = true;
      }
    }

    if (last_pulse[ch] != st->current) {
      last_pulse[ch] = st->current;
      any_change = true;
    }

    out[ch] = st->current;
  }

  if (any_change) {
    servo_pca9685_write_batch(out);
  }

  if (first_ramp_active) {
    bool done = true;
    for (uint8_t ch = 0; ch < SERVO_NUM_CHANNELS; ch++) {
      servo_cfg_t *cfg = &configs[ch];
      if (!cfg->enabled) continue;
      if ((int8_t)ch == ramp_exempt_ch) continue;
      if (!states[ch].arrived) {
        done = false;
        break;
      }
    }
    if (done) first_ramp_active = false;
  }
}
