#include "servo_pca9685.h"
#include <Wire.h>

#define PCA9685_MODE1      0x00
#define PCA9685_PRESCALE   0xFE
#define PCA9685_LED0_ON_L  0x06

static void pca9685_write8(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(SERVO_I2C_ADDR);
  Wire.write(reg);
  Wire.write(value);
  uint8_t err = Wire.endTransmission();
  if (err != 0) {
    Serial.printf("I2C write err reg=0x%02X val=0x%02X err=%d\n", reg, value, err);
  }
}

static uint8_t pca9685_read8(uint8_t reg) {
  Wire.beginTransmission(SERVO_I2C_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(SERVO_I2C_ADDR, (uint8_t)1);
  return Wire.read();
}

void servo_pca9685_init() {
  Serial.println("PCA9685: initializing I2C...");
  Wire.begin(SERVO_SDA_PIN, SERVO_SCL_PIN);
  Wire.setClock(100000);

  uint8_t scan_err = Wire.endTransmission();
  Serial.printf("PCA9685: SDA=%d SCL=%d addr=0x%02X\n", SERVO_SDA_PIN, SERVO_SCL_PIN, SERVO_I2C_ADDR);

  Wire.beginTransmission(SERVO_I2C_ADDR);
  scan_err = Wire.endTransmission();
  Serial.printf("PCA9685: device scan result: %s\n", scan_err == 0 ? "FOUND" : "NOT FOUND");

  pca9685_write8(PCA9685_MODE1, 0x00);

  float prescale = 25000000.0;
  prescale /= 4096.0;
  prescale /= (float)SERVO_FREQ;
  prescale -= 1.0;
  uint8_t prescale_val = (uint8_t)prescale;
  Serial.printf("PCA9685: prescale=%d (target %dHz)\n", prescale_val, SERVO_FREQ);

  uint8_t old_mode = pca9685_read8(PCA9685_MODE1);
  Serial.printf("PCA9685: MODE1 before=0x%02X\n", old_mode);

  uint8_t new_mode = (old_mode & 0x7F) | 0x10;
  pca9685_write8(PCA9685_MODE1, new_mode);
  pca9685_write8(PCA9685_PRESCALE, prescale_val);
  pca9685_write8(PCA9685_MODE1, old_mode);
  delay(5);
  pca9685_write8(PCA9685_MODE1, old_mode | 0xA0);

  uint8_t mode_after = pca9685_read8(PCA9685_MODE1);
  Serial.printf("PCA9685: MODE1 after=0x%02X\n", mode_after);

  servo_pca9685_off();
  Serial.println("PCA9685: init done");
}

void servo_pca9685_set(uint8_t channel, uint16_t pulse) {
  if (channel >= SERVO_CHANNELS) return;
  uint8_t reg = PCA9685_LED0_ON_L + 4 * channel;
  Wire.beginTransmission(SERVO_I2C_ADDR);
  Wire.write(reg);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.write(pulse & 0xFF);
  Wire.write((pulse >> 8) & 0x0F);
  Wire.endTransmission();
}

void servo_pca9685_set_all(uint16_t pulse) {
  Wire.beginTransmission(SERVO_I2C_ADDR);
  Wire.write(PCA9685_LED0_ON_L);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.write(pulse & 0xFF);
  Wire.write((pulse >> 8) & 0x0F);
  Wire.endTransmission();
}

void servo_pca9685_off() {
  for (uint8_t ch = 0; ch < SERVO_CHANNELS; ch++) {
    uint8_t reg = PCA9685_LED0_ON_L + 4 * ch;
    Wire.beginTransmission(SERVO_I2C_ADDR);
    Wire.write(reg);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.endTransmission();
  }
}
