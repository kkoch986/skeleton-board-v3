#include "servo_pca9685.h"
#include <Wire.h>
#include "i2c_bus.h"

#define PCA9685_MODE1       0x00
#define PCA9685_MODE2       0x01
#define PCA9685_PRESCALE    0xFE
#define PCA9685_LED0_ON_L   0x06

#define PCA9685_SW_RESET    0x06

static void pca9685_write8(uint8_t reg, uint8_t value) {
  if (!i2c_bus_lock(50)) return;
  Wire.beginTransmission(SERVO_I2C_ADDR);
  Wire.write(reg);
  Wire.write(value);
  uint8_t err = Wire.endTransmission();
  i2c_bus_unlock();
  if (err != 0) {
    Serial.printf("I2C write err reg=0x%02X val=0x%02X err=%d\n", reg, value, err);
  }
}

static uint8_t pca9685_read8(uint8_t reg) {
  if (!i2c_bus_lock(50)) return 0xFF;
  Wire.beginTransmission(SERVO_I2C_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((int)SERVO_I2C_ADDR, (int)1);
  uint8_t v = Wire.read();
  i2c_bus_unlock();
  return v;
}

void servo_pca9685_init() {
  Serial.println("PCA9685: initializing...");
  i2c_bus_init();

  pinMode(SERVO_OE_PIN, OUTPUT);
  digitalWrite(SERVO_OE_PIN, LOW);

  Wire.begin(SERVO_SDA_PIN, SERVO_SCL_PIN);
  Wire.setClock(400000);

  i2c_bus_lock(50);
  Wire.beginTransmission(SERVO_I2C_ADDR);
  uint8_t scan_err = Wire.endTransmission();
  i2c_bus_unlock();
  Serial.printf("PCA9685: SDA=%d SCL=%d addr=0x%02X %s\n",
                SERVO_SDA_PIN, SERVO_SCL_PIN, SERVO_I2C_ADDR,
                scan_err == 0 ? "FOUND" : "NOT FOUND");

  i2c_bus_lock(50);
  Wire.beginTransmission(0x00);
  Wire.write(PCA9685_SW_RESET);
  Wire.endTransmission();
  i2c_bus_unlock();
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
  if (!i2c_bus_lock(50)) return;
  uint8_t reg = PCA9685_LED0_ON_L + 4 * channel;
  Wire.beginTransmission(SERVO_I2C_ADDR);
  Wire.write(reg);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.write(pulse & 0xFF);
  Wire.write((pulse >> 8) & 0x0F);
  Wire.endTransmission();
  i2c_bus_unlock();
}

/* Write all 16 channels in a single auto-increment transaction
   (MODE1 bit 5 / AI is set during init). */
void servo_pca9685_write_batch(const uint16_t *pulses) {
  if (!i2c_bus_lock(50)) return;
  Wire.beginTransmission(SERVO_I2C_ADDR);
  Wire.write(PCA9685_LED0_ON_L);
  for (uint8_t ch = 0; ch < SERVO_CHANNELS; ch++) {
    uint16_t p = pulses[ch];
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(p & 0xFF);
    Wire.write((p >> 8) & 0x0F);
  }
  Wire.endTransmission();
  i2c_bus_unlock();
}

void servo_pca9685_set_all(uint16_t pulse) {
  for (uint8_t ch = 0; ch < SERVO_CHANNELS; ch++) {
    servo_pca9685_set(ch, pulse);
  }
}

void servo_pca9685_off() {
  if (!i2c_bus_lock(50)) return;
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
  i2c_bus_unlock();
}

void servo_pca9685_enable(bool on) {
  digitalWrite(SERVO_OE_PIN, on ? LOW : HIGH);
}
