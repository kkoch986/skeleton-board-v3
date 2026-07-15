#include <Arduino.h>
#include "status_led.h"
#include "envelope_detect.h"
#include "board_id.h"
#include "servo_pca9685.h"

#define SWEEP_STEP 5
#define SWEEP_DELAY 20

void setup() {
  Serial.begin(115200);
  delay(1000);

  status_led_init();
  envelope_detect_init();
  board_id_init();
  servo_pca9685_init();

  Serial.println("skeleton-board-v3 servo test");

  status_led_set(0, 0, 255);
  delay(300);
  status_led_off();
}

void loop() {
  for (uint16_t pos = SERVO_MIN_PULSE; pos <= SERVO_MAX_PULSE; pos += SWEEP_STEP) {
    for (uint8_t ch = 0; ch < SERVO_CHANNELS; ch++) {
      servo_pca9685_set(ch, pos);
    }
    Serial.printf("servo pos %d/%d\n", pos, SERVO_MAX_PULSE);
    delay(SWEEP_DELAY);
  }

  for (int16_t pos = SERVO_MAX_PULSE; pos >= SERVO_MIN_PULSE; pos -= SWEEP_STEP) {
    for (uint8_t ch = 0; ch < SERVO_CHANNELS; ch++) {
      servo_pca9685_set(ch, pos);
    }
    Serial.printf("servo pos %d/%d\n", pos, SERVO_MAX_PULSE);
    delay(SWEEP_DELAY);
  }
}
