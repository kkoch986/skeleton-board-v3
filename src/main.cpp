#include <Arduino.h>
#include "status_led.h"
#include "board_id.h"
#include "envelope_detect.h"
#include "servo_pca9685.h"
#include "dmx_rx.h"
#include "expansion.h"
#include "config.h"
#include "control.h"
#include "status.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  status_led_init();
  board_id_init();
  envelope_detect_init();
  expansion_init();

  Serial.printf("Board ID: %u\n", board_id_read());

  servo_pca9685_init();
  servo_power_enable_all(true);

  dmx_rx_init();
  control_init();

  Serial.println("skeleton-board-v3 ready (press boot button for WiFi config)");
  status_init();
}

void loop() {
  config_update();
  control_update();
  status_update();
}
