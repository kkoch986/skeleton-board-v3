#include "servo_pca9685.h"
#include <Wire.h>

#define PCA9685_MODE1       0x00
#define PCA9685_MODE2       0x01
#define PCA9685_PRESCALE    0xFE
#define PCA9685_LED0_ON_L   0x06

#define PCA9685_SW_RESET    0x06

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
  Wire.requestFrom((int)SERVO_I2C_ADDR, (int)1);
  return Wire.read();
}

void servo_pca9685_init() {
  Serial.println("PCA9685: initializing...");

  pinMode(SERVO_OE_PIN, OUTPUT);
  digitalWrite(SERVO_OE_PIN, LOW);

  pinMode(SERVO_POWER_BANK0_PIN, OUTPUT);
  pinMode(SERVO_POWER_BANK1_PIN, OUTPUT);
  servo_power_enable_all(false);

  Wire.begin(SERVO_SDA_PIN, SERVO_SCL_PIN);
  Wire.setClock(100000);

  Wire.beginTransmission(SERVO_I2C_ADDR);
  uint8_t scan_err = Wire.endTransmission();
  Serial.printf("PCA9685: SDA=%d SCL=%d addr=0x%02X %s\n",
                SERVO_SDA_PIN, SERVO_SCL_PIN, SERVO_I2C_ADDR,
                scan_err == 0 ? "FOUND" : "NOT FOUND");

  Wire.beginTransmission(0x00);
  Wire.write(PCA9685_SW_RESET);
  Wire.endTransmission();
  delay(10);

  pca9685_write8(PCA9685_MODE1, 0xA0);
  pca9685_write8(PCA9685_MODE2, 0x04);
  delay(10);

  pca9685_write8(PCA9685_MODE1, 0x10);
  pca9685_write8(PCA9685_PRESCALE, 121);
  delay(10);

  pca9685_write8(PCA9685_MODE1, 0xA0);
  delay(10);

  uint8_t mode1 = pca9685_read8(PCA9685_MODE1);
  uint8_t mode2 = pca9685_read8(PCA9685_MODE2);
  Serial.printf("PCA9685: MODE1=0x%02X MODE2=0x%02X\n", mode1, mode2);

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
  for (uint8_t ch = 0; ch < SERVO_CHANNELS; ch++) {
    servo_pca9685_set(ch, pulse);
  }
}

void servo_pca9685_off() {
  for (uint8_t ch = 0; ch < SERVO_CHANNELS; ch++) {
    uint8_t reg = PCA9685_LED0_ON_L + 4 * ch;
    Wire.beginTransmission(SERVO_I2C_ADDR);
    Wire.write(reg);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0x10);
    Wire.endTransmission();
  }
}

void servo_power_enable(uint8_t bank, bool on) {
  uint8_t pin = (bank == 0) ? SERVO_POWER_BANK0_PIN : SERVO_POWER_BANK1_PIN;
  digitalWrite(pin, on ? HIGH : LOW);
}

void servo_power_enable_all(bool on) {
  servo_power_enable(0, on);
  servo_power_enable(1, on);
}
