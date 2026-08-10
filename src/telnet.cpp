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
#include "dmx_map.h"
#include "eye_control.h"
#include "envelope_control.h"
#include "envelope_detect.h"
#include <Wire.h>
#include "i2c_bus.h"

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
  client.println("  dmx                     show mapped eye/servo dmx values");
  client.println("  dmx raw                 dump non-zero raw dmx channels");
  client.println("  dmx all                 full dmx table (1-512)");
  client.println("  dmx offset [val]        show or set dmx offset (1-512)");
  client.println("  servo                   show all servo config and state");
  client.println("  servo <ch>              show servo ch config (ch 0-15)");
  client.println("  servo <ch> move <pos>   move servo (102-512, manual override)");
  client.println("  servo <ch> auto         resume dmx control");
  client.println("  servo <ch> lower <val>  set lower limit (102-512)");
  client.println("  servo <ch> upper <val>  set upper limit (102-512)");
  client.println("  servo <ch> center <val> set center (102-512)");
  client.println("  servo <ch> smooth <val> set smoothing (0-255, 0=instant)");
  client.println("  servo <ch> enable       enable servo");
  client.println("  servo <ch> disable      disable servo");
  client.println("  servo <ch> invert <on|off> flip DMX direction");
  client.println("  servo <ch> label <str>  set label (max 15 chars)");
  client.println("  servo <ch> save         save config to flash");
  client.println("  servo all on            enable all servo outputs (OE LOW)");
  client.println("  servo all off           disable all servo outputs (OE HIGH)");
  client.println("  eye                     show eye status");
  client.println("  eye look <x> <y>        move eye (-100 to 100)");
  client.println("  eye jump <x> <y>        jump eye (-100 to 100)");
  client.println("  eye idle                eye idle animation");
  client.println("  eye blink [ms]          blink (default 200ms)");
  client.println("  eye squint <0-255>      set squint level");
  client.println("  eye sclera <r> <g> <b>  set sclera color (0-255)");
  client.println("  eye iris <r> <g> <b>    set iris color (0-255)");
  client.println("  eye autoblink <on|off>  toggle auto blink");
  client.println("  eye blinkspeed <ms>     set auto blink interval");
  client.println("  eye smooth <0-255>      set eye smoothing");
  client.println("  eye mode <0|1|2>       set eye mode (0=normal,1=sprite)");
  client.println("  eye spriteix <id>       set sprite index");
  client.println("  eye reset               reset eye board");
  client.println("  eye wifi ssid <ssid>    set eye wifi ssid");
  client.println("  eye wifi pass <pass>    set eye wifi password");
  client.println("  eye wifi connect        connect eye to wifi (stored creds)");
  client.println("  eye wifi forget         clear eye wifi credentials");
  client.println("  eye wifi status         show eye wifi status");
  client.println("  i2c scan                scan i2c bus for devices");
  client.println("  i2c scan v              scan with verbose error codes");
  client.println("  i2c scanretry [n]       scan n times, 1s apart");
  client.println("  i2c read <addr> <reg> [len]  read register(s)");
  client.println("  i2c write <addr> <reg> <val> [val2...]  write register(s)");
  client.println("  envelope               show envelope config and live amplitude");
  client.println("  envelope enable        enable envelope→servo control");
  client.println("  envelope disable       disable envelope→servo control");
  client.println("  envelope servo <ch>    set target servo (0-15)");
  client.println("  envelope min <pulse>   servo position at zero amplitude");
  client.println("  envelope max <pulse>   servo position at max amplitude");
  client.println("  envelope invert <on|off> flip amplitude direction");
  client.println("  envelope baseline      recalibrate DC offset (be quiet!)");
  client.println("  envelope gain <0-1023> amplitude gain (64=1.0x, 1023=16x)");
  client.println("  envelope smooth <0-255> amplitude smoothing (0=instant)");
  client.println("  envelope interval <ms>  update interval (default 20)");
  client.println("  envelope test          live amplitude readout (3s)");
  client.println("  envelope calibrate     watch amplitude range (5s, sets min/max)");
  client.println("  envelope monitor       live bar graph (10s, Ctrl+C to stop)");
  client.println("  led <r> <g> <b>         set status led (0-255)");
  client.println("  debug                  show debug mode state");
  client.println("  debug on               enable debug mode (persists, boots to WiFi)");
  client.println("  debug off              disable debug mode (boots to DMX)");
  client.println("  restart                 reboot device");
}

static void cmd_servo_table() {
  uint16_t offset = config_get_dmx_offset();
  client.println("ch  en  label          dmx  lo   hi   ctr  smth pos%  tgt% arr man");
  for (uint8_t ch = 0; ch < SERVO_NUM_CHANNELS; ch++) {
    servo_cfg_t   *cfg = servo_config_get(ch);
    servo_state_t *st  = servo_state_get(ch);
    uint8_t pct_cur = 0, pct_tgt = 0;
    if (cfg->upper_limit > cfg->lower_limit) {
      pct_cur = (uint8_t)((uint32_t)(st->current - cfg->lower_limit) * 100 /
                          (cfg->upper_limit - cfg->lower_limit));
      pct_tgt = (uint8_t)((uint32_t)(st->target - cfg->lower_limit) * 100 /
                          (cfg->upper_limit - cfg->lower_limit));
    }
    client.printf("%2u  %s  %-14s %3u %4u %4u %4u  %3u  %3u%% %3u%%  %s   %s\n",
                  ch, cfg->enabled ? " Y" : " N", cfg->label,
                  offset + DMX_EYE_LEN + ch,
                  cfg->lower_limit, cfg->upper_limit, cfg->center,
                  cfg->smoothing, pct_cur, pct_tgt,
                  st->arrived ? "Y" : "N",
                  st->manual ? "Y" : "N");
  }
}

static void cmd_servo_detail(uint8_t ch) {
  servo_cfg_t   *cfg = servo_config_get(ch);
  servo_state_t *st  = servo_state_get(ch);
  if (!cfg || !st) { client.println("error: bad channel"); return; }

  uint8_t pct_cur = 0, pct_tgt = 0;
  if (cfg->upper_limit > cfg->lower_limit) {
    pct_cur = (uint8_t)((uint32_t)(st->current - cfg->lower_limit) * 100 /
                        (cfg->upper_limit - cfg->lower_limit));
    pct_tgt = (uint8_t)((uint32_t)(st->target - cfg->lower_limit) * 100 /
                        (cfg->upper_limit - cfg->lower_limit));
  }

  uint16_t offset = config_get_dmx_offset();
  client.printf("servo %u: \"%s\"\n", ch, cfg->label);
  client.printf("  enabled:  %s\n", cfg->enabled ? "yes" : "no");
  client.printf("  dmx ch:   %u\n", offset + DMX_EYE_LEN + ch);
  client.printf("  limits:   %u - %u\n", cfg->lower_limit, cfg->upper_limit);
  client.printf("  center:   %u\n", cfg->center);
  client.printf("  smoothing: %u\n", cfg->smoothing);
  client.printf("  invert:   %s\n", cfg->invert ? "yes" : "no");
  client.printf("  position: %u -> %u  (%u%% -> %u%%)%s%s\n",
                st->current, st->target, pct_cur, pct_tgt,
                st->arrived ? " (arrived)" : "",
                st->manual ? " (manual)" : "");
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
    servo_state_get(ch)->manual = true;
    servo_config_set_target(ch, pos);
    client.printf("servo %u -> %u (manual)\n", ch, pos);
  } else if (strcmp(args, "auto") == 0) {
    servo_state_get(ch)->manual = false;
    client.printf("servo %u dmx control\n", ch);
  } else if (strncmp(args, "lower ", 6) == 0) {
    cfg->lower_limit = atoi(args + 6);
    servo_config_save(ch);
    client.printf("servo %u lower = %u\n", ch, cfg->lower_limit);
  } else if (strncmp(args, "upper ", 6) == 0) {
    cfg->upper_limit = atoi(args + 6);
    servo_config_save(ch);
    client.printf("servo %u upper = %u\n", ch, cfg->upper_limit);
  } else if (strncmp(args, "center ", 7) == 0) {
    cfg->center = atoi(args + 7);
    servo_config_save(ch);
    client.printf("servo %u center = %u\n", ch, cfg->center);
  } else if (strncmp(args, "smooth ", 7) == 0) {
    cfg->smoothing = atoi(args + 7);
    servo_config_save(ch);
    client.printf("servo %u smoothing = %u\n", ch, cfg->smoothing);
  } else if (strcmp(args, "enable") == 0) {
    cfg->enabled = true;
    servo_config_save(ch);
    client.printf("servo %u enabled\n", ch);
  } else if (strcmp(args, "disable") == 0) {
    cfg->enabled = false;
    servo_config_save(ch);
    client.printf("servo %u disabled\n", ch);
  } else if (strncmp(args, "invert ", 7) == 0) {
    cfg->invert = (strcmp(args + 7, "on") == 0);
    servo_config_save(ch);
    client.printf("servo %u invert = %s\n", ch, cfg->invert ? "on" : "off");
  } else if (strncmp(args, "label ", 6) == 0) {
    strncpy(cfg->label, args + 6, SERVO_LABEL_LEN - 1);
    cfg->label[SERVO_LABEL_LEN - 1] = '\0';
    servo_config_save(ch);
    client.printf("servo %u label = \"%s\"\n", ch, cfg->label);
  } else if (strcmp(args, "save") == 0) {
    servo_config_save(ch);
    client.printf("servo %u saved\n", ch);
  } else {
    client.printf("unknown: servo %s\n", args);
  }
}

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

static void cmd_i2c(const char *args) {
  if (strncmp(args, "scanretry", 9) == 0) {
    uint8_t attempts = 5;
    if (args[9] == ' ') attempts = atoi(args + 10);
    if (attempts < 1) attempts = 1;
    if (attempts > 20) attempts = 20;
    client.printf("scanning i2c bus (%u attempts, 1s apart)...\n", attempts);
    uint8_t last_found = 0;
    for (uint8_t a = 0; a < attempts; a++) {
      uint8_t found = 0;
      if (i2c_bus_lock(50)) {
        for (uint8_t addr = 1; addr < 127; addr++) {
          Wire.beginTransmission(addr);
          if (Wire.endTransmission() == 0) found++;
        }
        i2c_bus_unlock();
      }
      client.printf("  attempt %u: found %u device(s)\n", a + 1, found);
      if (found > last_found) last_found = found;
      if (a < attempts - 1) delay(1000);
    }
    client.printf("max devices found: %u\n", last_found);
  } else if (strncmp(args, "scan", 4) == 0) {
    bool verbose = (strcmp(args, "scan v") == 0);
    client.println("scanning i2c bus...");
    uint8_t found = 0;
    if (i2c_bus_lock(50)) {
      for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
          client.printf("  0x%02X  OK\n", addr);
          found++;
        } else if (verbose) {
          client.printf("  0x%02X  err=%d\n", addr, err);
        }
      }
      i2c_bus_unlock();
    }
    client.printf("found %u device(s)\n", found);
    if (verbose) {
      client.println("err: 1=len,2=addr nack,3=data nack,4=other,5=timeout");
    }
  } else if (strncmp(args, "read ", 5) == 0) {
    uint8_t addr = 0, reg = 0, len = 1;
    sscanf(args + 5, "%hhu %hhu %hhu", &addr, &reg, &len);
    if (len > 32) len = 32;
    if (i2c_bus_lock(50)) {
      Wire.beginTransmission(addr);
      Wire.write(reg);
      Wire.endTransmission(false);
      uint8_t got = Wire.requestFrom((int)addr, (int)len);
      client.printf("0x%02X reg 0x%02X [%u byte(s)]:", addr, reg, got);
      for (uint8_t i = 0; i < got; i++) {
        client.printf(" %02X", Wire.read());
      }
      client.println();
      i2c_bus_unlock();
    }
  } else if (strncmp(args, "write ", 6) == 0) {
    uint8_t addr = 0, reg = 0;
    const char *p = args + 6;
    sscanf(p, "%hhu %hhu", &addr, &reg);
    while (*p && *p != ' ') p++;
    while (*p == ' ') p++;
    while (*p && *p != ' ') p++;
    while (*p == ' ') p++;
    if (i2c_bus_lock(50)) {
      Wire.beginTransmission(addr);
      Wire.write(reg);
      while (*p) {
        uint8_t val = 0;
        sscanf(p, "%hhu", &val);
        Wire.write(val);
        while (*p && *p != ' ') p++;
        while (*p == ' ') p++;
      }
      uint8_t err = Wire.endTransmission();
      i2c_bus_unlock();
      client.printf("0x%02X reg 0x%02X write: %s\n", addr, reg,
                    err == 0 ? "ok" : "error");
    }
  } else {
    client.println("usage: i2c scan | i2c read <addr> <reg> [len] | i2c write <addr> <reg> <val>...");
  }
}

static void cmd_eye(const char *args) {
  if (strlen(args) == 0) {
    eye_status_t st;
    if (eye_read_status(&st, EYE_DEFAULT_ADDR)) {
      client.printf("eye: x=%d y=%d cur_x=%d cur_y=%d\n",
                    st.x, st.y, st.current_x, st.current_y);
      client.printf("  squint=%u blink=%ums auto_blink=%s interval=%ums\n",
                    st.squint, st.blink_duration_ms,
                    st.auto_blink ? "on" : "off", st.auto_blink_interval_ms);
      client.printf("  smoothing=%u/255 ext_ctrl=%s sprite=%s auto=%s sprite_ix=%d\n",
                    (uint8_t)(st.smoothing * 255),
                    st.external_control ? "on" : "off",
                    st.sprite_mode ? "on" : "off",
                    st.autonomous ? "on" : "off",
                    st.sprite_index == EYE_SPRITE_NONE ? -1 : st.sprite_index);
      client.printf("  sclera=0x%04X iris_med=0x%04X iris_dark=0x%04X\n",
                    st.sclera_color, st.iris_med_color, st.iris_dark_color);
      client.printf("  curve falloff=%u/255 min=%u/255 closure=%u/255\n",
                    (uint8_t)(st.curve_falloff * 255),
                    (uint8_t)(st.curve_minimum * 255),
                    (uint8_t)(st.closure_strength * 255));
      client.printf("  i2c_init=%s master=%s ota_flags=0x%02X\n",
                    st.i2c_initialized ? "yes" : "no",
                    st.i2c_master_detected ? "yes" : "no",
                    st.ota_flags);
    } else {
      client.println("eye: not found");
    }
    return;
  }

  if (strncmp(args, "look ", 5) == 0) {
    int x = 0, y = 0;
    sscanf(args + 5, "%d %d", &x, &y);
    eye_look(x, y);
    client.printf("eye look %d %d\n", x, y);
  } else if (strncmp(args, "jump ", 5) == 0) {
    int x = 0, y = 0;
    sscanf(args + 5, "%d %d", &x, &y);
    eye_jump(x, y);
    client.printf("eye jump %d %d\n", x, y);
  } else if (strcmp(args, "idle") == 0) {
    eye_idle();
    client.println("eye idle");
  } else if (strncmp(args, "blink", 5) == 0) {
    uint16_t ms = 200;
    if (args[5] == ' ') ms = atoi(args + 6);
    eye_blink(ms);
    client.printf("eye blink %ums\n", ms);
  } else if (strncmp(args, "squint ", 7) == 0) {
    uint8_t level = atoi(args + 7);
    eye_squint(level);
    client.printf("eye squint %u\n", level);
  } else if (strncmp(args, "sclera ", 7) == 0) {
    uint8_t r = 0, g = 0, b = 0;
    sscanf(args + 7, "%hhu %hhu %hhu", &r, &g, &b);
    eye_sclera_rgb(rgb565(r, g, b));
    client.printf("eye sclera %u %u %u\n", r, g, b);
  } else if (strncmp(args, "iris ", 5) == 0) {
    uint8_t r = 0, g = 0, b = 0;
    sscanf(args + 5, "%hhu %hhu %hhu", &r, &g, &b);
    eye_iris_rgb(rgb565(r, g, b));
    client.printf("eye iris %u %u %u\n", r, g, b);
  } else if (strncmp(args, "autoblink ", 10) == 0) {
    bool on = strcmp(args + 10, "on") == 0;
    eye_auto_blink(on);
    client.printf("eye autoblink %s\n", on ? "on" : "off");
  } else if (strncmp(args, "blinkspeed ", 11) == 0) {
    uint16_t ms = atoi(args + 11);
    eye_auto_blink_speed(ms);
    client.printf("eye blinkspeed %ums\n", ms);
  } else if (strncmp(args, "smooth ", 7) == 0) {
    uint8_t level = atoi(args + 7);
    eye_smoothing(level);
    client.printf("eye smooth %u\n", level);
  } else if (strncmp(args, "mode ", 5) == 0) {
    uint8_t mode = atoi(args + 5);
    eye_sprite_mode(mode);
    client.printf("eye mode %u\n", mode);
  } else if (strncmp(args, "spriteix ", 9) == 0) {
    uint8_t id = atoi(args + 9);
    eye_sprite_index(id);
    client.printf("eye spriteix %u\n", id);
  } else if (strcmp(args, "reset") == 0) {
    eye_reset();
    client.println("eye reset");
  } else if (strncmp(args, "wifi ", 5) == 0) {
    const char *w = args + 5;
    if (strncmp(w, "ssid ", 5) == 0) {
      eye_wifi_ssid(w + 5);
      client.printf("eye wifi ssid = %s\n", w + 5);
    } else if (strncmp(w, "pass ", 5) == 0) {
      eye_wifi_pass(w + 5);
      client.printf("eye wifi pass = %s\n", w + 5);
    } else if (strcmp(w, "connect") == 0) {
      eye_wifi_connect();
      client.println("eye wifi: connecting...");
    } else if (strcmp(w, "forget") == 0) {
      eye_wifi_forget();
      client.println("eye wifi: credentials cleared");
    } else if (strcmp(w, "status") == 0) {
      uint8_t s = eye_wifi_status();
      const char *state = s == 2 ? "connected" : s == 1 ? "connecting" : s == 0 ? "disconnected" : "unknown";
      client.printf("eye wifi: %s (%u)\n", state, s);
    } else {
      client.println("usage: eye wifi <ssid|pass|connect|forget|status>");
    }
  } else {
    client.printf("unknown: eye %s\n", args);
  }
}

static void cmd_process(const char *cmd) {
  if (strcmp(cmd, "help") == 0) {
    cmd_help();
  } else if (strcmp(cmd, "status") == 0) {
    client.printf("id=%u wifi=%s dmx=%s offset=%u\n",
                  board_id_read(),
                  config_wifi_connected() ? "yes" : "no",
                  dmx_rx_active() ? "yes" : "no",
                  config_get_dmx_offset());
  } else if (strcmp(cmd, "id") == 0) {
    client.printf("board id: %u\n", board_id_read());
  } else if (strncmp(cmd, "dmx", 3) == 0) {
    const char *args = cmd + 3;
    while (*args == ' ') args++;
    uint16_t offset = config_get_dmx_offset();

    if (strcmp(args, "all") == 0) {
      client.println("ch   val  label");
      for (uint16_t ch = 1; ch <= DMX_MAX_CHANNELS; ch++) {
        uint16_t val = dmx_rx_get(ch);
        const char *label = "";
        uint16_t rel = ch - offset;
        if (rel < DMX_EYE_LEN) {
          static const char *eye_names[] = {
            "sclera_r", "sclera_g", "sclera_b",
            "iris_r", "iris_g", "iris_b",
            "eye_x", "eye_y", "squint",
            "auto_blink", "blink_spd",
            "eye_mode", "sprite_ix"
          };
          if (rel < DMX_EYE_LEN) label = eye_names[rel];
        } else if (rel >= DMX_SERVO_BASE && rel < DMX_SERVO_BASE + SERVO_NUM_CHANNELS) {
          uint8_t sv = rel - DMX_SERVO_BASE;
          servo_cfg_t *cfg = servo_config_get(sv);
          if (cfg && cfg->label[0]) label = cfg->label;
        }
        if (val > 0 || label[0])
          client.printf("%3u  %3u  %s\n", ch, val, label);
      }
    } else if (strcmp(args, "raw") == 0) {
      for (uint16_t ch = 1; ch <= DMX_MAX_CHANNELS; ch++) {
        uint16_t val = dmx_rx_get(ch);
        if (val > 0) client.printf("ch%u=%u\n", ch, val);
      }
    } else if (strncmp(args, "offset ", 7) == 0) {
      bool ok = config_set_dmx_offset(atoi(args + 7));
      client.printf("dmx offset = %u (%s)\n", config_get_dmx_offset(),
                    ok ? "saved" : "save FAILED");
    } else if (strcmp(args, "offset") == 0) {
      client.printf("dmx offset = %u\n", offset);
    } else {
      client.printf("eye (offset %u):\n", offset);
      static const char *eye_names[] = {
        "sclera_r", "sclera_g", "sclera_b",
        "iris_r", "iris_g", "iris_b",
        "eye_x", "eye_y", "squint",
        "auto_blink", "blink_spd",
        "eye_mode", "sprite_ix"
      };
      for (uint8_t i = 0; i < DMX_EYE_LEN; i++) {
        client.printf("  %3u  %-12s %3u\n", offset + i, eye_names[i], dmx_rx_get(offset + i));
      }
      client.println("servos:");
      for (uint8_t i = 0; i < SERVO_NUM_CHANNELS; i++) {
        servo_cfg_t *cfg = servo_config_get(i);
        uint16_t val = dmx_rx_get(offset + DMX_SERVO_BASE + i);
        client.printf("  %3u  %-14s %3u\n", offset + DMX_SERVO_BASE + i,
                      cfg->label[0] ? cfg->label : "(unnamed)", val);
      }
    }
  } else if (strncmp(cmd, "servo", 5) == 0) {
    const char *args = cmd + 5;
    while (*args == ' ') args++;

    if (strlen(args) == 0) {
      cmd_servo_table();
    } else if (strncmp(args, "all ", 4) == 0) {
      const char *a = args + 4;
      if (strcmp(a, "on") == 0) { servo_pca9685_enable(true); client.println("servos: enabled (OE LOW)"); }
      else if (strcmp(a, "off") == 0) { servo_pca9685_enable(false); client.println("servos: disabled (OE HIGH)"); }
      else { client.println("usage: servo all <on|off>"); }
    } else {
      uint8_t ch = atoi(args);
      while (*args && *args != ' ') args++;
      while (*args == ' ') args++;
      cmd_servo(ch, args);
    }
  } else if (strncmp(cmd, "eye", 3) == 0) {
    const char *args = cmd + 3;
    while (*args == ' ') args++;
    cmd_eye(args);
  } else if (strncmp(cmd, "i2c", 3) == 0) {
    const char *args = cmd + 3;
    while (*args == ' ') args++;
    cmd_i2c(args);
  } else if (strncmp(cmd, "envelope", 8) == 0) {
    const char *args = cmd + 8;
    while (*args == ' ') args++;
    envelope_ctrl_cfg_t *ec = envelope_ctrl_get();

    if (strlen(args) == 0) {
      client.printf("envelope: %s\n", ec->enabled ? "ON" : "OFF");
      client.printf("  servo:  %u\n", ec->servo_ch);
      client.printf("  min:    %u\n", ec->min_pulse);
      client.printf("  max:    %u\n", ec->max_pulse);
      client.printf("  invert: %s\n", ec->invert ? "yes" : "no");
      client.printf("  gain:   %u (64=1.0x, 1023=16x)\n", ec->gain);
      client.printf("  smooth: %u (0=instant)\n", ec->smoothing);
      client.printf("  interval: %u ms\n", ec->interval_ms);
      client.printf("  detect: %s\n", envelope_detect_connected() ? "YES" : "no");
      client.printf("  amplitude: %u\n", envelope_detect_amplitude());
    } else if (strcmp(args, "enable") == 0) {
      ec->enabled = true;
      envelope_ctrl_save();
      client.printf("envelope: ON → servo %u\n", ec->servo_ch);
    } else if (strcmp(args, "disable") == 0) {
      ec->enabled = false;
      envelope_ctrl_save();
      client.println("envelope: OFF");
    } else if (strncmp(args, "servo ", 6) == 0) {
      uint8_t ch = atoi(args + 6);
      if (ch < SERVO_NUM_CHANNELS) {
        ec->servo_ch = ch;
        envelope_ctrl_save();
        client.printf("envelope: servo = %u\n", ch);
      } else {
        client.println("servo: 0-15");
      }
    } else if (strncmp(args, "min ", 4) == 0) {
      uint16_t v = atoi(args + 4);
      ec->min_pulse = v;
      envelope_ctrl_save();
      client.printf("envelope: min = %u\n", v);
    } else if (strncmp(args, "max ", 4) == 0) {
      uint16_t v = atoi(args + 4);
      ec->max_pulse = v;
      envelope_ctrl_save();
      client.printf("envelope: max = %u\n", v);
    } else if (strncmp(args, "invert ", 7) == 0) {
      ec->invert = (strcmp(args + 7, "on") == 0);
      envelope_ctrl_save();
      client.printf("envelope: invert = %s\n", ec->invert ? "on" : "off");
    } else if (strcmp(args, "baseline") == 0) {
      client.println("envelope: recalibrating... stay quiet for 2s");
      delay(2000);
      envelope_detect_init();
      client.printf("envelope: DC offset calibrated, amplitude now %u\n",
                    envelope_detect_amplitude());
    } else if (strncmp(args, "gain ", 5) == 0) {
      uint16_t g = atoi(args + 5);
      if (g > 1023) g = 1023;
      ec->gain = g;
      envelope_detect_set_gain(g);
      envelope_ctrl_save();
      client.printf("envelope: gain = %u (64=1.0x, 1023=16x)\n", g);
    } else if (strcmp(args, "gain") == 0) {
      client.printf("envelope: gain = %u (64=1.0x, 1023=16x)\n", ec->gain);
    } else if (strncmp(args, "smooth ", 7) == 0) {
      uint8_t s = atoi(args + 7);
      ec->smoothing = s;
      envelope_detect_set_smoothing(s);
      envelope_ctrl_save();
      client.printf("envelope: smooth = %u (0=instant)\n", s);
    } else if (strcmp(args, "smooth") == 0) {
      client.printf("envelope: smooth = %u (0=instant)\n", ec->smoothing);
    } else if (strncmp(args, "interval ", 9) == 0) {
      uint16_t ms = atoi(args + 9);
      ec->interval_ms = ms;
      envelope_ctrl_save();
      client.printf("envelope: interval = %u ms\n", ms);
    } else if (strcmp(args, "interval") == 0) {
      client.printf("envelope: interval = %u ms\n", ec->interval_ms);
    } else if (strcmp(args, "test") == 0) {
      client.println("envelope test (3s, Ctrl+C to abort):");
      for (int i = 0; i < 30; i++) {
        client.printf("  amplitude: %u\n", envelope_detect_amplitude());
        delay(100);
      }
    } else if (strcmp(args, "calibrate") == 0) {
      client.println("envelope calibrate (5s, make noise!):");
      uint16_t lo = 255, hi = 0;
      for (int i = 0; i < 50; i++) {
        uint16_t amp = envelope_detect_amplitude();
        if (amp < lo) lo = amp;
        if (amp > hi) hi = amp;
        client.printf("  %u  (min=%u max=%u)\n", amp, lo, hi);
        delay(100);
      }
      ec->min_pulse = SERVO_MIN_PULSE;
      ec->max_pulse = SERVO_MAX_PULSE;
      envelope_ctrl_save();
      client.printf("envelope calibrate: observed amplitude range %u-%u\n", lo, hi);
      client.printf("  servo min/max unchanged (%u/%u) — use 'envelope min/max' to map\n",
                    ec->min_pulse, ec->max_pulse);
    } else if (strcmp(args, "monitor") == 0) {
      client.println("envelope monitor (10s, Ctrl+C to stop)");
      client.printf("  servo ch %u (%s)  min=%u max=%u\n", ec->servo_ch,
                    ec->invert ? "invert" : "normal",
                    ec->min_pulse, ec->max_pulse);
      client.println("  raw  amp  tgt  cur  0                 255");
      for (int i = 0; i < 100; i++) {
        uint16_t raw = envelope_detect_amplitude_raw();
        uint16_t amp = envelope_detect_amplitude();
        servo_state_t *st = servo_state_get(ec->servo_ch);
        uint16_t tgt = st ? st->target : 0;
        uint16_t cur = st ? st->current : 0;
        uint8_t bars = map(amp, 0, 255, 0, 40);
        char line[44];
        memset(line, ' ', 41);
        for (uint8_t j = 0; j < bars; j++) line[j] = '#';
        line[41] = '\0';
        client.printf("%3u  %3u  %3u  %3u |%s|\n", raw, amp, tgt, cur, line);
        delay(100);
      }
    } else {
      client.println("usage: envelope [enable|disable|servo|min|max|invert|baseline|test|calibrate|monitor]");
    }
  } else if (strncmp(cmd, "led ", 4) == 0) {
    uint8_t r = 0, g = 0, b = 0;
    sscanf(cmd + 4, "%hhu %hhu %hhu", &r, &g, &b);
    status_led_set(r, g, b);
    client.printf("led = %u %u %u\n", r, g, b);
  } else if (strcmp(cmd, "debug") == 0) {
    client.printf("debug mode: %s (reboot to %s)\n",
                  config_get_debug() ? "ON" : "OFF",
                  config_get_debug() ? "keep WiFi" : "skip WiFi");
  } else if (strcmp(cmd, "debug on") == 0) {
    config_set_debug(true);
    client.println("debug mode: ON (will boot to WiFi)");
  } else if (strcmp(cmd, "debug off") == 0) {
    config_set_debug(false);
    client.println("debug mode: OFF (will boot to DMX)");
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
