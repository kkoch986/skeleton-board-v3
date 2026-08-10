#include "control.h"
#include <Arduino.h>
#include "dmx_rx.h"
#include "dmx_map.h"
#include "servo_config.h"
#include "eye_control.h"
#include "config.h"
#include "status_led.h"
#include "envelope_detect.h"
#include "servo_pca9685.h"
#include "envelope_control.h"
#include "board_id.h"

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

static bool dmx_signal_active = false;

#define SERVO_TASK_PERIOD_MS 10

static void control_servo_task(void *arg) {
  TickType_t last = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&last, pdMS_TO_TICKS(SERVO_TASK_PERIOD_MS));
    servo_config_update();
    envelope_ctrl_apply();
  }
}

void control_init() {
  servo_config_init();
  envelope_ctrl_init();

  eye_init(EYE_DEFAULT_ADDR);
  if (eye_probe(EYE_DEFAULT_ADDR)) {
    Serial.println("Eye board found");
    eye_auto_blink(true);
    eye_idle();
  } else {
    Serial.println("Eye board NOT found");
  }

  xTaskCreatePinnedToCore(control_servo_task, "servo_ctrl", 4096, NULL, 4, NULL, 1);
}

void control_update() {
  if (!dmx_rx_frame_ready()) {
    if (!dmx_rx_active()) dmx_signal_active = false;
    return;
  }

  if (!dmx_signal_active) {
    dmx_signal_active = true;
    servo_config_begin_first_ramp();
  }

  status_led_set(0, 255, 0);

  uint16_t offset = config_get_dmx_offset();

  static uint8_t  c_mode       = 0xFF;
  static int16_t  c_look_x     = INT16_MIN;
  static int16_t  c_look_y     = INT16_MIN;
  static uint8_t  c_squint     = 0xFF;
  static uint16_t c_sclera     = 0xFFFF;
  static uint16_t c_iris       = 0xFFFF;
  static uint8_t  c_auto_blink = 0xFF;
  static uint16_t c_blink_spd  = 0xFFFF;
  static uint8_t  c_sprite_ix  = 0xFF;

  uint8_t eye_mode = dmx_rx_get(offset + DMX_EYE_MODE);

  if (eye_mode != c_mode) {
    c_mode = eye_mode;
    if (eye_mode == 2) {
      eye_sprite_mode(1);
      c_sprite_ix = 0xFF;
    } else {
      eye_sprite_mode(0);
      if (eye_mode == 0) {
        eye_idle();
      } else {
        c_look_x = INT16_MIN;
        c_look_y = INT16_MIN;
      }
    }
  }

  /* iris travel is constrained to +/-60 on the eye board */
  if (eye_mode == 1) {
    int16_t eye_x = (int16_t)map(dmx_rx_get(offset + DMX_EYE_X), 0, 255, -60, 60);
    int16_t eye_y = (int16_t)map(dmx_rx_get(offset + DMX_EYE_Y), 0, 255, -60, 60);
    if (eye_x != c_look_x || eye_y != c_look_y) {
      eye_look(eye_x, eye_y);
      c_look_x = eye_x;
      c_look_y = eye_y;
    }
  }

  uint8_t squint = dmx_rx_get(offset + DMX_EYE_SQUINT);
  if (squint != c_squint) {
    eye_squint(squint);
    c_squint = squint;
  }

  uint16_t sclera = rgb565(
    dmx_rx_get(offset + DMX_EYE_SCLERA_R),
    dmx_rx_get(offset + DMX_EYE_SCLERA_G),
    dmx_rx_get(offset + DMX_EYE_SCLERA_B));
  if (sclera != c_sclera) {
    eye_sclera_rgb(sclera);
    c_sclera = sclera;
  }

  uint16_t iris = rgb565(
    dmx_rx_get(offset + DMX_EYE_IRIS_R),
    dmx_rx_get(offset + DMX_EYE_IRIS_G),
    dmx_rx_get(offset + DMX_EYE_IRIS_B));
  if (iris != c_iris) {
    eye_iris_rgb(iris);
    c_iris = iris;
  }

  uint8_t auto_blink = dmx_rx_get(offset + DMX_EYE_AUTO_BLINK) > 0;
  if (auto_blink != c_auto_blink) {
    eye_auto_blink(auto_blink);
    c_auto_blink = auto_blink;
  }

  uint16_t blink_spd = (uint16_t)map(dmx_rx_get(offset + DMX_EYE_BLINK_SPD), 0, 255, 0, 30000);
  if (blink_spd != c_blink_spd) {
    eye_auto_blink_speed(blink_spd);
    c_blink_spd = blink_spd;
  }

  if (eye_mode == 2) {
    uint8_t sprite_ix = dmx_rx_get(offset + DMX_EYE_SPRITE_IX);
    if (sprite_ix != c_sprite_ix) {
      eye_sprite_index(sprite_ix);
      c_sprite_ix = sprite_ix;
    }
  }

  for (uint8_t i = 0; i < SERVO_NUM_CHANNELS; i++) {
    servo_cfg_t *cfg = servo_config_get(i);
    if (!cfg || !cfg->enabled) continue;
    servo_state_t *st = servo_state_get(i);
    if (st && st->manual) continue;
    envelope_ctrl_cfg_t *ec = envelope_ctrl_get();
    if (envelope_ctrl_active() && ec->servo_ch == i) continue;
    uint16_t dmx_val = dmx_rx_get(offset + DMX_SERVO_BASE + i);
    uint16_t pulse;
    if (cfg->invert) {
      pulse = map(dmx_val, 0, 255, cfg->upper_limit, cfg->lower_limit);
    } else {
      pulse = map(dmx_val, 0, 255, cfg->lower_limit, cfg->upper_limit);
    }
    servo_config_set_target(i, pulse);
  }

  if (dmx_rx_get(DMX_CHAN_WIFI_MODE) == board_id_read() + 1) {
    config_enable_wifi();
  }
}
