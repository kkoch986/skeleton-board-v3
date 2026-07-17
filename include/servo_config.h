#ifndef SERVO_CONFIG_H
#define SERVO_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define SERVO_NUM_CHANNELS 16
#define SERVO_LABEL_LEN    16

typedef struct {
  uint16_t lower_limit;
  uint16_t upper_limit;
  uint16_t center;
  uint8_t  smoothing;
  bool     enabled;
  char     label[SERVO_LABEL_LEN];
} servo_cfg_t;

typedef struct {
  uint16_t current;
  uint16_t target;
  bool     arrived;
} servo_state_t;

void     servo_config_init();
void     servo_config_update();
void     servo_config_load(uint8_t ch);
void     servo_config_save(uint8_t ch);
void     servo_config_load_all();
void     servo_config_save_all();

servo_cfg_t   *servo_config_get(uint8_t ch);
servo_state_t *servo_state_get(uint8_t ch);

void     servo_config_set_target(uint8_t ch, uint16_t target);
uint16_t servo_config_apply_limits(uint8_t ch, uint16_t value);

#endif
