#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Arduino.h>

#define STATUS_LED_R_PIN 10
#define STATUS_LED_G_PIN 9
#define STATUS_LED_B_PIN 11

#define STATUS_LED_RESOLUTION 8
#define STATUS_LED_FREQUENCY 5000

void status_led_init();
void status_led_set(uint8_t r, uint8_t g, uint8_t b);
void status_led_off();

#endif
