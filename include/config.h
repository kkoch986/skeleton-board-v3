#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define CFG_AP_NAME       "skeleton-board"
#define CFG_AP_PASSWORD   "skeleton"
#define CFGPortal_TIMEOUT 180
#define CFG_BUTTON_PIN    0

void config_init();
void config_update();
bool config_wifi_enabled();
bool config_wifi_connected();

const char *config_get_ssid();
const char *config_get_pass();
uint16_t config_get_dmx_offset();

#endif
