#include "servo_config.h"
#include <Arduino.h>
#include <Preferences.h>
#include "servo_pca9685.h"

static Preferences prefs;
static servo_cfg_t  configs[SERVO_NUM_CHANNELS];
static servo_state_t states[SERVO_NUM_CHANNELS];

static void defaults(servo_cfg_t *cfg) {
  cfg->lower_limit = SERVO_MIN_PULSE;
  cfg->upper_limit = SERVO_MAX_PULSE;
  cfg->center      = (SERVO_MIN_PULSE + SERVO_MAX_PULSE) / 2;
  cfg->smoothing   = 0;
  cfg->enabled     = false;
  cfg->label[0]    = '\0';
}

static void nvs_key(uint8_t ch, char *buf) {
  sprintf(buf, "sv%u", ch);
}

void servo_config_init() {
  for (uint8_t ch = 0; ch < SERVO_NUM_CHANNELS; ch++) {
    defaults(&configs[ch]);
    states[ch].current = configs[ch].center;
    states[ch].target  = configs[ch].center;
    states[ch].arrived = true;
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

  states[ch].current = configs[ch].center;
  states[ch].target  = configs[ch].center;
  states[ch].arrived = true;
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

void servo_config_update() {
  for (uint8_t ch = 0; ch < SERVO_NUM_CHANNELS; ch++) {
    servo_cfg_t   *cfg  = &configs[ch];
    servo_state_t *st   = &states[ch];

    if (!cfg->enabled) continue;
    if (st->arrived) continue;

    if (cfg->smoothing == 0 || abs((int)st->current - (int)st->target) <= cfg->smoothing) {
      st->current = st->target;
    } else if (st->current < st->target) {
      st->current += cfg->smoothing;
    } else {
      st->current -= cfg->smoothing;
    }

    if (st->current == st->target) {
      st->arrived = true;
    }

    servo_pca9685_set(ch, st->current);
  }
}
