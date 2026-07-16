#include "dmx_rx.h"
#include <Arduino.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define DMX_UART_NUM    UART_NUM_1
#define DMX_BAUD        250000
#define DMX_RX_BUF_SIZE 2048
#define DMX_TX_BUF_SIZE 0
#define DMX_QUEUE_SIZE  16
#define DMX_FRAME_SIZE  513

static uint16_t dmx_channels[DMX_MAX_CHANNELS];
static uint16_t dmx_start_code;
static volatile bool dmx_frame_ready;
static volatile uint32_t dmx_last_frame_ms;
static QueueHandle_t dmx_uart_queue;

static void dmx_task(void *arg) {
  uart_event_t event;
  uint8_t rxBuf[DMX_FRAME_SIZE + 64];

  for (;;) {
    if (xQueueReceive(dmx_uart_queue, &event, pdMS_TO_TICKS(100))) {
      switch (event.type) {
        case UART_DATA: {
          if (!event.timeout_flag) break;

          int total = uart_read_bytes(DMX_UART_NUM, rxBuf, sizeof(rxBuf),
                                      pdMS_TO_TICKS(5));
          if (total < DMX_FRAME_SIZE) break;

          int offset = total - DMX_FRAME_SIZE;
          dmx_start_code = rxBuf[offset];
          if (dmx_start_code != 0x00) break;

          for (uint16_t i = 0; i < DMX_MAX_CHANNELS; i++) {
            dmx_channels[i] = rxBuf[offset + 1 + i];
          }
          dmx_frame_ready = true;
          dmx_last_frame_ms = millis();
          break;
        }

        case UART_BREAK:
        case UART_FRAME_ERR:
          break;

        case UART_FIFO_OVF:
          uart_flush_input(DMX_UART_NUM);
          break;

        default:
          break;
      }
    }
  }
}

void dmx_rx_init() {
  for (uint16_t i = 0; i < DMX_MAX_CHANNELS; i++) {
    dmx_channels[i] = 0;
  }
  dmx_frame_ready = false;
  dmx_last_frame_ms = 0;
  dmx_start_code = 0;

  uart_config_t uart_config = {};
  uart_config.baud_rate = DMX_BAUD;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_2;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.source_clk = UART_SCLK_APB;

  uart_driver_install(DMX_UART_NUM, DMX_RX_BUF_SIZE, DMX_TX_BUF_SIZE,
                      DMX_QUEUE_SIZE, &dmx_uart_queue, 0);
  uart_param_config(DMX_UART_NUM, &uart_config);
  uart_set_pin(DMX_UART_NUM, UART_PIN_NO_CHANGE, DMX_RX_PIN,
               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

  /* RX idle timeout: fire after 2 byte times (~88µs) of silence */
  /* this detects the gap between DMX frames */
  uart_set_rx_timeout(DMX_UART_NUM, 2);

  xTaskCreatePinnedToCore(dmx_task, "dmx_rx", 4096, NULL, 5, NULL, 1);
}

bool dmx_rx_frame_ready() {
  if (dmx_frame_ready) {
    dmx_frame_ready = false;
    return true;
  }
  return false;
}

bool dmx_rx_active() {
  return (millis() - dmx_last_frame_ms) < 500;
}

uint16_t dmx_rx_get(uint16_t channel) {
  if (channel < 1 || channel > DMX_MAX_CHANNELS) return 0;
  return dmx_channels[channel - 1];
}

uint16_t dmx_rx_get_start_code() {
  return dmx_start_code;
}
