#include "expansion.h"

static const uint8_t expansion_pins[EXPANSION_NUM_PINS] = {
  EXPANSION_PIN_0, EXPANSION_PIN_1, EXPANSION_PIN_2, EXPANSION_PIN_3,
  EXPANSION_PIN_4, EXPANSION_PIN_5, EXPANSION_PIN_6, EXPANSION_PIN_7
};

void expansion_init() {
  for (int i = 0; i < EXPANSION_NUM_PINS; i++) {
    pinMode(expansion_pins[i], INPUT_PULLDOWN);
  }
  analogReadResolution(EXPANSION_ADC_RESOLUTION);
}

bool expansion_digital_read(uint8_t index) {
  if (index >= EXPANSION_NUM_PINS) return false;
  return digitalRead(expansion_pins[index]) == HIGH;
}

uint16_t expansion_analog_read(uint8_t index) {
  if (index >= EXPANSION_NUM_PINS) return 0;
  return analogRead(expansion_pins[index]);
}
