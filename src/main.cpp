#include <Arduino.h>
#include "status_led.h"
#include "envelope_detect.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  status_led_init();
  envelope_detect_init();

  Serial.println("skeleton-board-v3 envelope detect test");

  status_led_set(0, 255, 0);
  delay(300);
  status_led_off();
}

void loop() {
  bool connected = envelope_detect_connected();
  uint8_t brightness = envelope_detect_amplitude();

  if (!connected) {
    status_led_off();
    return;
  }

  status_led_set(0, brightness, 0);

  static uint32_t last_print = 0;
  if (millis() - last_print > 100) {
    Serial.printf("detect=%d brightness=%d\n", connected, brightness);
    last_print = millis();
  }
}
