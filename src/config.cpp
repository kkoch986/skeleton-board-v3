#include "config.h"
#include <Arduino.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include "status_led.h"
#include "board_id.h"
#include "telnet.h"

static WiFiManager wm;

#define CFG_PREFS_NS "sb"

static Preferences prefs;
static uint16_t dmx_offset = 1;
static bool wifi_enabled = false;
static bool debug_mode = false;
static uint32_t last_button_press = 0;

static void config_portal_run() {
  char ap_name[32];
  snprintf(ap_name, sizeof(ap_name), "%s-%u", CFG_AP_NAME, board_id_read());

  Serial.printf("WiFi: connecting (hold button for config portal)...\n");
  status_led_set(0, 0, 255);

  if (!wm.autoConnect(ap_name, CFG_AP_PASSWORD)) {
    Serial.println("WiFi: failed to connect");
    status_led_off();
    return;
  }

  wifi_enabled = true;
  Serial.printf("WiFi: connected to %s\n", WiFi.SSID().c_str());
  status_led_set(0, 255, 0);

  ArduinoOTA.setHostname(ap_name);
  ArduinoOTA.onStart([]() {
    Serial.println("OTA: start");
    status_led_set(255, 0, 255);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA: done");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error [%u]\n", error);
  });
  ArduinoOTA.begin();
  Serial.printf("OTA: ready\n");

  telnet_init();
}

void config_init() {
  prefs.begin(CFG_PREFS_NS, true);
  uint16_t loaded = 0;
  if (prefs.getBytes("dmx_offset", &loaded, sizeof(loaded)) == sizeof(loaded)) {
    dmx_offset = loaded;
  }
  uint8_t dv = 0;
  if (prefs.getBytes("debug", &dv, sizeof(dv)) == sizeof(dv)) {
    debug_mode = dv != 0;
  }
  prefs.end();

  Serial.printf("WiFi: loaded dmx_offset=%u debug=%d\n", dmx_offset, debug_mode);

  wm.setConfigPortalTimeout(CFGPortal_TIMEOUT);
  wm.setMinimumSignalQuality(15);

  WiFi.mode(WIFI_STA);

  pinMode(CFG_BUTTON_PIN, INPUT_PULLUP);

  if (debug_mode) {
    Serial.println("Debug mode: enabling WiFi");
    config_enable_wifi();
  }
}

void config_enable_wifi() {
  if (!wifi_enabled) {
    Serial.println("WiFi: triggered (remote)");
    config_portal_run();
  }
}

void config_update() {
  if (!wifi_enabled && digitalRead(CFG_BUTTON_PIN) == LOW &&
      millis() - last_button_press > 300) {
    last_button_press = millis();
    Serial.println("Button pressed — starting WiFi config portal");
    config_portal_run();
  }

  if (wifi_enabled) {
    ArduinoOTA.handle();
    telnet_update();
  }
}

bool config_wifi_enabled() {
  return wifi_enabled;
}

bool config_wifi_connected() {
  return WiFi.status() == WL_CONNECTED;
}

const char *config_get_ssid() {
  static String ssid;
  ssid = WiFi.SSID();
  return ssid.c_str();
}

const char *config_get_pass() {
  static String pass;
  pass = WiFi.psk();
  return pass.c_str();
}

uint16_t config_get_dmx_offset() {
  return dmx_offset;
}

bool config_set_dmx_offset(uint16_t offset) {
  if (offset < 1) offset = 1;
  if (offset > 512) offset = 512;
  dmx_offset = offset;

  prefs.begin(CFG_PREFS_NS, false);
  size_t w = prefs.putBytes("dmx_offset", &dmx_offset, sizeof(dmx_offset));
  uint16_t back = 0;
  size_t r = prefs.getBytes("dmx_offset", &back, sizeof(back));
  prefs.end();
  Serial.printf("dmx_offset save: wrote=%u read=%u val=%u\n",
                (unsigned)w, (unsigned)r, (unsigned)back);
  bool ok = w == sizeof(dmx_offset) && r == sizeof(back) && back == dmx_offset;
  Serial.printf("Config saved: dmx_offset=%u ok=%s\n",
                dmx_offset, ok ? "yes" : "FAIL");
  return ok;
}

void config_set_debug(bool on) {
  debug_mode = on;
  uint8_t v = on ? 1 : 0;
  prefs.begin(CFG_PREFS_NS, false);
  size_t w = prefs.putBytes("debug", &v, sizeof(v));
  uint8_t back = 0;
  size_t r = prefs.getBytes("debug", &back, sizeof(back));
  prefs.end();
  bool ok = w == sizeof(v) && r == sizeof(back) && back == v;
  Serial.printf("DEBUG: saved to '%s' ns, on=%d ok=%s\n",
                CFG_PREFS_NS, on, ok ? "yes" : "FAIL");
}

bool config_get_debug() {
  return debug_mode;
}
