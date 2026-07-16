#include "status.h"
#include <Arduino.h>
#include "board_id.h"
#include "envelope_detect.h"
#include "dmx_rx.h"
#include "expansion.h"
#include "config.h"
#include "status_led.h"

static uint32_t last_status = 0;

void status_init() {
  last_status = 0;
}

void status_update() {
  if (millis() - last_status < 1000) return;

  uint8_t id = board_id_read();
  uint16_t amp = envelope_detect_amplitude();
  bool env = envelope_detect_connected();

  Serial.printf("ID=%u ENV=%s AMP=%u ", id, env ? "Y" : "N", amp);

  Serial.printf("EXP:");
  for (uint8_t i = 0; i < EXPANSION_NUM_PINS; i++) {
    Serial.printf(" %u", expansion_digital_read(i));
  }

  Serial.printf("  DMX=%s", dmx_rx_active() ? "Y" : "N");
  for (uint16_t ch = 1; ch <= DMX_MAX_CHANNELS; ch++) {
    uint16_t val = dmx_rx_get(ch);
    if (val > 0) {
      Serial.printf(" ch%u=%u", ch, val);
    }
  }

  Serial.printf(" WiFi=%s\n",
                config_wifi_enabled() ? (config_wifi_connected() ? "Y" : "N") : "off");

  if (!dmx_rx_active()) {
    status_led_off();
  }

  last_status = millis();
}
