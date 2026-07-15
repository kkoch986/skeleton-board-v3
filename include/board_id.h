#ifndef BOARD_ID_H
#define BOARD_ID_H

#include <Arduino.h>

#define BOARD_ID_BIT0_PIN 15
#define BOARD_ID_BIT1_PIN 16
#define BOARD_ID_BIT2_PIN 17
#define BOARD_ID_BIT3_PIN 18

#define BOARD_ID_NUM_PINS 4

void board_id_init();
uint8_t board_id_read();

#endif
