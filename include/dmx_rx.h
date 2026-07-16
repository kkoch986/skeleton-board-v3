#ifndef DMX_RX_H
#define DMX_RX_H

#include <stdint.h>

#define DMX_RX_PIN       13
#define DMX_MAX_CHANNELS 512

void dmx_rx_init();
bool dmx_rx_frame_ready();
bool dmx_rx_active();
uint16_t dmx_rx_get(uint16_t channel);
uint16_t dmx_rx_get_start_code();

#endif
