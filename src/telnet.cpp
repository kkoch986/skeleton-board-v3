#include "telnet.h"
#include <Arduino.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include "status_led.h"
#include "board_id.h"
#include "dmx_rx.h"
#include "servo_pca9685.h"
#include "expansion.h"
#include "config.h"

static WiFiServer server(TELNET_PORT);
static WiFiClient client;
static char cmd_buf[128];
static uint8_t cmd_len = 0;

static void cmd_process(const char *cmd) {
  if (strcmp(cmd, "help") == 0) {
    client.println("commands:");
    client.println("  help              show this list");
    client.println("  status            board id, wifi, dmx state");
    client.println("  id                print board id");
    client.println("  dmx               dump non-zero dmx channels");
    client.println("  servo <ch> <us>   set servo 0-15 to 1000-2000us");
    client.println("  led <r> <g> <b>   set status led (0-255)");
    client.println("  restart           reboot device");
  } else if (strcmp(cmd, "status") == 0) {
    client.printf("id=%u wifi=%s dmx=%s\n",
                  board_id_read(),
                  config_wifi_connected() ? "yes" : "no",
                  dmx_rx_active() ? "yes" : "no");
  } else if (strcmp(cmd, "id") == 0) {
    client.printf("board id: %u\n", board_id_read());
  } else if (strncmp(cmd, "dmx", 3) == 0) {
    for (uint16_t ch = 1; ch <= DMX_MAX_CHANNELS; ch++) {
      uint16_t val = dmx_rx_get(ch);
      if (val > 0) client.printf("ch%u=%u\n", ch, val);
    }
  } else if (strncmp(cmd, "servo ", 6) == 0) {
    uint8_t ch = 0;
    uint16_t us = 1500;
    sscanf(cmd + 6, "%hhu %hu", &ch, &us);
    if (ch >= SERVO_CHANNELS) {
      client.println("error: channel 0-15");
      return;
    }
    uint16_t pulse = map(us, 1000, 2000, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
    servo_pca9685_set(ch, pulse);
    client.printf("servo ch%u = %u us (pulse %u)\n", ch, us, pulse);
  } else if (strncmp(cmd, "led ", 4) == 0) {
    uint8_t r = 0, g = 0, b = 0;
    sscanf(cmd + 4, "%hhu %hhu %hhu", &r, &g, &b);
    status_led_set(r, g, b);
    client.printf("led = %u %u %u\n", r, g, b);
  } else if (strcmp(cmd, "restart") == 0) {
    client.println("restarting...");
    delay(100);
    ESP.restart();
  } else if (strlen(cmd) > 0) {
    client.printf("unknown: %s\n", cmd);
  }
}

void telnet_init() {
  server.begin();
  cmd_len = 0;
}

void telnet_update() {
  if (!client || !client.connected()) {
    client = server.available();
    if (client) {
      client.println("\nskeleton-board-v3 telnet");
      client.println("type 'help' for commands");
      client.print("> ");
      cmd_len = 0;
    }
    return;
  }

  while (client.available()) {
    char c = client.read();
    if (c == '\n' || c == '\r') {
      if (cmd_len > 0) {
        cmd_buf[cmd_len] = '\0';
        cmd_process(cmd_buf);
        cmd_len = 0;
      }
      client.print("> ");
    } else if (cmd_len < sizeof(cmd_buf) - 1) {
      cmd_buf[cmd_len++] = c;
    }
  }
}
