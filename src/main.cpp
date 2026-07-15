#include <Arduino.h>
#include "status_led.h"
#include "envelope_detect.h"
#include "board_id.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  status_led_init();
  envelope_detect_init();
  board_id_init();

  Serial.println("skeleton-board-v3 test");

  status_led_set(0, 255, 0);
  delay(300);
  status_led_off();
}

void loop() {
  bool connected = envelope_detect_connected();
  uint8_t brightness = envelope_detect_amplitude();
  uint8_t board_id = board_id_read();

  static uint32_t last_print = 0;
  if (millis() - last_print > 100) {
    Serial.printf("detect=%d brightness=%d board_id=%d (0b%04b)\n",
                  connected, brightness, board_id, board_id);
    last_print = millis();
  }

  if (!connected) {
    status_led_off();
    return;
  }

  status_led_set(0, brightness, 0);
}
