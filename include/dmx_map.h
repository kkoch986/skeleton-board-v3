#ifndef DMX_MAP_H
#define DMX_MAP_H

/* Eye channels: 6 color channels first, then control channels. */
#define DMX_EYE_SCLERA_R   0
#define DMX_EYE_SCLERA_G   1
#define DMX_EYE_SCLERA_B   2
#define DMX_EYE_IRIS_R     3
#define DMX_EYE_IRIS_G     4
#define DMX_EYE_IRIS_B     5
#define DMX_EYE_X          6
#define DMX_EYE_Y          7
#define DMX_EYE_SQUINT     8
#define DMX_EYE_AUTO_BLINK 9
#define DMX_EYE_BLINK_SPD  10
/* 0 = idle (autonomous random look), 1 = normal (direct X/Y), 2 = sprite */
#define DMX_EYE_MODE       11
#define DMX_EYE_SPRITE_IX  12
#define DMX_EYE_LEN        13

#define DMX_SERVO_BASE     DMX_EYE_LEN
#define DMX_SERVO_LEN      16

#define DMX_CHAN_WIFI_MODE  512

#endif
