#include "config.h"
#include <Arduino.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include "status_led.h"
#include "telnet.h"

static WiFiManager wm;
static WiFiManagerParameter custom_dmx_offset("dmx_offset", "DMX Offset (1-512)", "1", 5);

static Preferences prefs;
static uint16_t dmx_offset = 1;
static bool wifi_enabled = false;
static uint32_t last_button_press = 0;

static void save_params_callback() {
  dmx_offset = atoi(custom_dmx_offset.getValue());
  if (dmx_offset < 1) dmx_offset = 1;
  if (dmx_offset > 512) dmx_offset = 512;

  prefs.begin("skeleton", false);
  prefs.putUShort("dmx_offset", dmx_offset);
  prefs.end();

  Serial.printf("Config saved: dmx_offset=%u\n", dmx_offset);
}

static void config_portal_run() {
  Serial.println("WiFi: starting config portal...");
  status_led_set(0, 0, 255);
  wm.setConfigPortalTimeout(CFGPortal_TIMEOUT);

  if (!wm.startConfigPortal(CFG_AP_NAME, CFG_AP_PASSWORD)) {
    Serial.println("WiFi: config portal timeout");
    status_led_off();
    return;
  }

  wifi_enabled = true;
  Serial.printf("WiFi: connected to %s\n", WiFi.SSID().c_str());
  status_led_set(0, 255, 0);
  telnet_init();
}

void config_init() {
  prefs.begin("skeleton", true);
  dmx_offset = prefs.getUShort("dmx_offset", 1);
  prefs.end();

  wm.setSaveParamsCallback(save_params_callback);
  wm.setConfigPortalTimeout(CFGPortal_TIMEOUT);
  wm.setMinimumSignalQuality(15);

  char def_offset[6];
  snprintf(def_offset, sizeof(def_offset), "%u", dmx_offset);
  custom_dmx_offset.setValue(def_offset, 5);
  wm.addParameter(&custom_dmx_offset);

  WiFi.mode(WIFI_STA);
  Serial.printf("WiFi: loaded dmx_offset=%u\n", dmx_offset);

  pinMode(CFG_BUTTON_PIN, INPUT_PULLUP);
}

void config_update() {
  if (!wifi_enabled && digitalRead(CFG_BUTTON_PIN) == LOW &&
      millis() - last_button_press > 300) {
    last_button_press = millis();
    Serial.println("Button pressed — starting WiFi config portal");
    config_init();
    config_portal_run();
  }

  if (wifi_enabled) {
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
