#include <Arduino.h>
#include <Wire.h>
#include "status_led.h"
#include "envelope_detect.h"
#include "board_id.h"
#include "servo_pca9685.h"

#define SWEEP_STEP 5
#define SWEEP_DELAY 20
#define I2C_DEBUG_INTERVAL 2000

static void i2c_scan() {
  Serial.println("--- I2C scan ---");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  found device at 0x%02X\n", addr);
    }
  }
  Serial.println("--- end scan ---");
}

static void i2c_debug_pca9685() {
  uint8_t modes[] = {0x00, 0xA0};
  for (int i = 0; i < 2; i++) {
    Wire.beginTransmission(SERVO_I2C_ADDR);
    Wire.write(0x00);
    Wire.endTransmission(false);
    Wire.requestFrom(SERVO_I2C_ADDR, (uint8_t)1);
    uint8_t mode1 = Wire.read();

    Wire.beginTransmission(SERVO_I2C_ADDR);
    Wire.write(0xFE);
    Wire.endTransmission(false);
    Wire.requestFrom(SERVO_I2C_ADDR, (uint8_t)1);
    uint8_t prescale = Wire.read();

    Serial.printf("PCA9685: MODE1=0x%02X PRESCALE=0x%02X\n", mode1, prescale);
  }
}

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
  static uint32_t last_debug = 0;
  if (millis() - last_debug >= I2C_DEBUG_INTERVAL) {
    i2c_scan();
    i2c_debug_pca9685();
    last_debug = millis();
  }

  for (uint16_t pos = SERVO_MIN_PULSE; pos <= SERVO_MAX_PULSE; pos += SWEEP_STEP) {
    for (uint8_t ch = 0; ch < SERVO_CHANNELS; ch++) {
      servo_pca9685_set(ch, pos);
    }
    delay(SWEEP_DELAY);
  }

  for (int16_t pos = SERVO_MAX_PULSE; pos >= SERVO_MIN_PULSE; pos -= SWEEP_STEP) {
    for (uint8_t ch = 0; ch < SERVO_CHANNELS; ch++) {
      servo_pca9685_set(ch, pos);
    }
    delay(SWEEP_DELAY);
  }
}
