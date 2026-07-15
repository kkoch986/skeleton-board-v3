#include <Arduino.h>
#include "status_led.h"

uint32_t hue_to_rgb(uint16_t hue) {
  uint8_t region = hue / 43;
  uint8_t remainder = (hue - (region * 43)) * 6;
  uint8_t p = 0;
  uint8_t q = (255 - remainder) * 255 / 255;
  uint8_t t = remainder * 255 / 255;

  switch (region) {
    case 0:  return (255 << 16) | (t << 8) | p;
    case 1:  return (q << 16) | (255 << 8) | p;
    case 2:  return (p << 16) | (255 << 8) | t;
    case 3:  return (p << 16) | (q << 8) | 255;
    case 4:  return (t << 16) | (p << 8) | 255;
    default: return (255 << 16) | (p << 8) | q;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  status_led_init();
  Serial.println("skeleton-board-v3 status LED test");
}

void loop() {
  for (uint16_t hue = 0; hue < 256; hue++) {
    uint32_t color = hue_to_rgb(hue);
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    status_led_set(r, g, b);
    delay(10);
  }
}
