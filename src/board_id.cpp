#include "board_id.h"

static const uint8_t board_id_pins[BOARD_ID_NUM_PINS] = {
  BOARD_ID_BIT0_PIN,
  BOARD_ID_BIT1_PIN,
  BOARD_ID_BIT2_PIN,
  BOARD_ID_BIT3_PIN
};

void board_id_init() {
  for (int i = 0; i < BOARD_ID_NUM_PINS; i++) {
    pinMode(board_id_pins[i], INPUT_PULLDOWN);
  }
}

uint8_t board_id_read() {
  uint8_t id = 0;
  for (int i = 0; i < BOARD_ID_NUM_PINS; i++) {
    if (digitalRead(board_id_pins[i]) == LOW) {
      id |= (1 << (BOARD_ID_NUM_PINS - 1 - i));
    }
  }
  return id;
}
