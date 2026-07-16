#ifndef SERVO_PCA9685_H
#define SERVO_PCA9685_H

#include <Arduino.h>

#define SERVO_SDA_PIN 3
#define SERVO_SCL_PIN 4
#define SERVO_I2C_ADDR 0x40

#define SERVO_MIN_PULSE 102
#define SERVO_MAX_PULSE 512
#define SERVO_CHANNELS 16
#define SERVO_POWER_BANK0_PIN  6
#define SERVO_POWER_BANK1_PIN  7
#define SERVO_OE_PIN           5

void servo_pca9685_init();
void servo_pca9685_set(uint8_t channel, uint16_t pulse);
void servo_pca9685_set_all(uint16_t pulse);
void servo_pca9685_off();
void servo_power_enable(uint8_t bank, bool on);
void servo_power_enable_all(bool on);

#endif
