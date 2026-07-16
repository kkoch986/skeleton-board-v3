#include "control.h"
#include <Arduino.h>
#include "dmx_rx.h"
#include "servo_pca9685.h"
#include "eye_control.h"
#include "config.h"
#include "status_led.h"

static uint32_t last_eye = 0;
static bool eye_on = false;

void control_init() {
  eye_init(EYE_DEFAULT_ADDR);
  if (eye_probe(EYE_DEFAULT_ADDR)) {
    Serial.println("Eye board found");
    eye_auto_blink(true);
    eye_idle();
  } else {
    Serial.println("Eye board NOT found");
  }
}

void control_update() {
  if (!dmx_rx_frame_ready()) return;

  status_led_set(0, 255, 0);

  uint16_t offset = config_get_dmx_offset();

  uint8_t servo_ch = dmx_rx_get(offset) >> 4;
  uint8_t eye_cmd = dmx_rx_get(offset + 1);

  if (servo_ch < SERVO_CHANNELS) {
    uint16_t pulse = map(dmx_rx_get(offset + 2), 0, 255, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
    servo_pca9685_set(servo_ch, pulse);
  }

  if (millis() - last_eye > 500) {
    if (eye_cmd == 1 && !eye_on) {
      eye_look(0, 0);
      eye_on = true;
      last_eye = millis();
    } else if (eye_cmd == 0 && eye_on) {
      eye_idle();
      eye_on = false;
      last_eye = millis();
    }
  }
}
