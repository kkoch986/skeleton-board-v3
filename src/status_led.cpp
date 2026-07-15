#include "status_led.h"

#define STATUS_LED_R_CHANNEL 0
#define STATUS_LED_G_CHANNEL 1
#define STATUS_LED_B_CHANNEL 2

void status_led_init() {
  ledcSetup(STATUS_LED_R_CHANNEL, STATUS_LED_FREQUENCY, STATUS_LED_RESOLUTION);
  ledcSetup(STATUS_LED_G_CHANNEL, STATUS_LED_FREQUENCY, STATUS_LED_RESOLUTION);
  ledcSetup(STATUS_LED_B_CHANNEL, STATUS_LED_FREQUENCY, STATUS_LED_RESOLUTION);

  ledcAttachPin(STATUS_LED_R_PIN, STATUS_LED_R_CHANNEL);
  ledcAttachPin(STATUS_LED_G_PIN, STATUS_LED_G_CHANNEL);
  ledcAttachPin(STATUS_LED_B_PIN, STATUS_LED_B_CHANNEL);

  status_led_off();
}

void status_led_set(uint8_t r, uint8_t g, uint8_t b) {
  ledcWrite(STATUS_LED_R_CHANNEL, 255 - r);
  ledcWrite(STATUS_LED_G_CHANNEL, 255 - g);
  ledcWrite(STATUS_LED_B_CHANNEL, 255 - b);
}

void status_led_off() {
  status_led_set(0, 0, 0);
}
