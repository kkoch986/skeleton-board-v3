#ifndef SERVO_PCA9685_H
#define SERVO_PCA9685_H

#include <Arduino.h>

#define SERVO_SDA_PIN 4
#define SERVO_SCL_PIN 5
#define SERVO_I2C_ADDR 0x40

#define SERVO_MIN_PULSE 102   // ~500us  (0 degrees)
#define SERVO_MAX_PULSE 512   // ~2500us (180 degrees)
#define SERVO_CHANNELS 16

#define SERVO_FREQ 50

void servo_pca9685_init();
void servo_pca9685_set(uint8_t channel, uint16_t pulse);
void servo_pca9685_set_all(uint16_t pulse);
void servo_pca9685_off();

#endif
