#include "i2c_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t i2c_mutex = NULL;

void i2c_bus_init() {
  if (!i2c_mutex) {
    i2c_mutex = xSemaphoreCreateMutex();
  }
}

bool i2c_bus_lock(uint32_t timeout_ms) {
  if (!i2c_mutex) return false;
  return xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void i2c_bus_unlock() {
  if (i2c_mutex) xSemaphoreGive(i2c_mutex);
}
