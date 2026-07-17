#include "telnet.h"
#include <Arduino.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include "status_led.h"
#include "board_id.h"
#include "dmx_rx.h"
#include "servo_pca9685.h"
#include "servo_config.h"
#include "expansion.h"
#include "config.h"

static WiFiServer server(TELNET_PORT);
static WiFiClient client;
static char cmd_buf[128];
static uint8_t cmd_len = 0;
static uint8_t iac_skip = 0;

static void cmd_help() {
  client.println("commands:");
  client.println("  help                    show this list");
  client.println("  status                  board id, wifi, dmx state");
  client.println("  id                      print board id");
  client.println("  dmx                     dump non-zero dmx channels");
  client.println("  servo                   show all servo config and state");
  client.println("  servo <ch>              show servo ch config and state");
  client.println("  servo <ch> move <pos>   move servo to position");
  client.println("  servo <ch> lower <val>  set lower limit");
  client.println("  servo <ch> upper <val>  set upper limit");
  client.println("  servo <ch> center <val> set center position");
  client.println("  servo <ch> smooth <val> set smoothing (0=instant)");
  client.println("  servo <ch> enable       enable servo");
  client.println("  servo <ch> disable      disable servo");
  client.println("  servo <ch> label <str>  set label");
  client.println("  servo <ch> save         save config to flash");
  client.println("  power                  show servo power state");
  client.println("  power <1|2> on         enable servo power bank");
  client.println("  power <1|2> off        disable servo power bank");
  client.println("  power all on           enable both power banks");
  client.println("  power all off          disable both power banks");
  client.println("  led <r> <g> <b>         set status led (0-255)");
  client.println("  restart                 reboot device");
}

static void cmd_servo_table() {
  client.println("ch  en  label          lo   hi   ctr  smth cur  tgt  arr");
  for (uint8_t ch = 0; ch < SERVO_NUM_CHANNELS; ch++) {
    servo_cfg_t   *cfg = servo_config_get(ch);
    servo_state_t *st  = servo_state_get(ch);
    client.printf("%2u  %s  %-14s %4u %4u %4u  %3u %4u %4u  %s\n",
                  ch, cfg->enabled ? " Y" : " N", cfg->label,
                  cfg->lower_limit, cfg->upper_limit, cfg->center,
                  cfg->smoothing, st->current, st->target,
                  st->arrived ? "Y" : "N");
  }
}

static void cmd_servo_detail(uint8_t ch) {
  servo_cfg_t   *cfg = servo_config_get(ch);
  servo_state_t *st  = servo_state_get(ch);
  if (!cfg || !st) { client.println("error: bad channel"); return; }

  client.printf("servo %u: \"%s\"\n", ch, cfg->label);
  client.printf("  enabled:  %s\n", cfg->enabled ? "yes" : "no");
  client.printf("  limits:   %u - %u\n", cfg->lower_limit, cfg->upper_limit);
  client.printf("  center:   %u\n", cfg->center);
  client.printf("  smoothing: %u\n", cfg->smoothing);
  client.printf("  position: %u -> %u%s\n", st->current, st->target, st->arrived ? " (arrived)" : "");
}

static void cmd_servo(uint8_t ch, const char *args) {
  if (ch >= SERVO_NUM_CHANNELS) { client.println("error: channel 0-15"); return; }

  if (strlen(args) == 0) {
    cmd_servo_detail(ch);
    return;
  }

  servo_cfg_t *cfg = servo_config_get(ch);

  if (strncmp(args, "move ", 5) == 0) {
    uint16_t pos = atoi(args + 5);
    servo_config_set_target(ch, pos);
    client.printf("servo %u -> %u\n", ch, pos);
  } else if (strncmp(args, "lower ", 6) == 0) {
    cfg->lower_limit = atoi(args + 6);
    client.printf("servo %u lower = %u\n", ch, cfg->lower_limit);
  } else if (strncmp(args, "upper ", 6) == 0) {
    cfg->upper_limit = atoi(args + 6);
    client.printf("servo %u upper = %u\n", ch, cfg->upper_limit);
  } else if (strncmp(args, "center ", 7) == 0) {
    cfg->center = atoi(args + 7);
    client.printf("servo %u center = %u\n", ch, cfg->center);
  } else if (strncmp(args, "smooth ", 7) == 0) {
    cfg->smoothing = atoi(args + 7);
    client.printf("servo %u smoothing = %u\n", ch, cfg->smoothing);
  } else if (strcmp(args, "enable") == 0) {
    cfg->enabled = true;
    client.printf("servo %u enabled\n", ch);
  } else if (strcmp(args, "disable") == 0) {
    cfg->enabled = false;
    servo_state_get(ch)->arrived = true;
    client.printf("servo %u disabled\n", ch);
  } else if (strncmp(args, "label ", 6) == 0) {
    strncpy(cfg->label, args + 6, SERVO_LABEL_LEN - 1);
    cfg->label[SERVO_LABEL_LEN - 1] = '\0';
    client.printf("servo %u label = \"%s\"\n", ch, cfg->label);
  } else if (strcmp(args, "save") == 0) {
    servo_config_save(ch);
    client.printf("servo %u saved\n", ch);
  } else {
    client.printf("unknown: servo %s\n", args);
  }
}

static void cmd_process(const char *cmd) {
  if (strcmp(cmd, "help") == 0) {
    cmd_help();
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
  } else if (strncmp(cmd, "servo", 5) == 0) {
    const char *args = cmd + 5;
    while (*args == ' ') args++;

    if (strlen(args) == 0) {
      cmd_servo_table();
    } else {
      uint8_t ch = atoi(args);
      while (*args && *args != ' ') args++;
      while (*args == ' ') args++;
      cmd_servo(ch, args);
    }
  } else if (strncmp(cmd, "power", 5) == 0) {
    const char *args = cmd + 5;
    while (*args == ' ') args++;

    if (strcmp(args, "all on") == 0) {
      servo_power_enable_all(true);
      client.println("power: both banks ON");
    } else if (strcmp(args, "all off") == 0) {
      servo_power_enable_all(false);
      client.println("power: both banks OFF");
    } else if (strncmp(args, "1 ", 2) == 0) {
      const char *a = args + 2;
      if (strcmp(a, "on") == 0) { servo_power_enable(0, true); client.println("power 1: ON"); }
      else if (strcmp(a, "off") == 0) { servo_power_enable(0, false); client.println("power 1: OFF"); }
      else { client.println("usage: power <1|2> <on|off>"); }
    } else if (strncmp(args, "2 ", 2) == 0) {
      const char *a = args + 2;
      if (strcmp(a, "on") == 0) { servo_power_enable(1, true); client.println("power 2: ON"); }
      else if (strcmp(a, "off") == 0) { servo_power_enable(1, false); client.println("power 2: OFF"); }
      else { client.println("usage: power <1|2> <on|off>"); }
    } else if (strlen(args) == 0) {
      client.printf("power 1: %s\n", digitalRead(SERVO_POWER_BANK0_PIN) ? "ON" : "OFF");
      client.printf("power 2: %s\n", digitalRead(SERVO_POWER_BANK1_PIN) ? "ON" : "OFF");
    } else {
      client.println("usage: power [all|1|2] [on|off]");
    }
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
      iac_skip = 0;
      cmd_len = 0;
      client.println("\nskeleton-board-v3 telnet");
      client.println("type 'help' for commands");
      client.print("> ");
    }
    return;
  }

  while (client.available()) {
    char c = client.read();

    if (iac_skip > 0) {
      iac_skip--;
      continue;
    }

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      if (cmd_len > 0) {
        cmd_buf[cmd_len] = '\0';
        cmd_process(cmd_buf);
        cmd_len = 0;
      }
      client.print("> ");
      continue;
    }

    if (c == '\xff') {
      iac_skip = 2;
      cmd_len = 0;
      continue;
    }

    if (c >= 0x20 && c <= 0x7e && cmd_len < sizeof(cmd_buf) - 1) {
      cmd_buf[cmd_len++] = c;
    }
  }
}
