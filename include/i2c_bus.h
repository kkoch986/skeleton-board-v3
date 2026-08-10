#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stdbool.h>
#include <stdint.h>

void i2c_bus_init();
bool i2c_bus_lock(uint32_t timeout_ms);
void i2c_bus_unlock();

#endif
