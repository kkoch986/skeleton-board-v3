# skeleton-board-v3

Custom ESP32-S3 board for skeleton prop control.

## Hardware

- ESP32-S3-WROOM-1-N8
- PCA9685 16-channel PWM servo driver (I2C 0x40)
- ISO3082DWR RS-485 transceiver (DMX512 input, IO13 via UART1)
- Common anode RGB status LED (R=IO10, G=IO9, B=IO11)
- Envelope detection circuit (IO1 ADC, IO2 detect)
- 4-bit board ID switches (IO15-IO18)
- Servo OE pin (IO5, active LOW)
- I2C eye board (SDA=IO3, SCL=IO4, addr 0x42 — shares the PCA9685 bus)
- Expansion pins IO35-IO42 (input only)

## DMX512 Channel Layout

DMX offset is configurable (1-512) via telnet (`dmx offset <n>`). Default offset is 1.

### Eye Block (offset +0 to +12)

| Channel | Function       | Range  | Mapping                                      |
|---------|----------------|--------|----------------------------------------------|
| +0      | Eye X          | 0-255  | 0=left(-100), 128=center, 255=right(+100)   |
| +1      | Eye Y          | 0-255  | 0=down(-100), 128=center, 255=up(+100)      |
| +2      | Squint         | 0-255  | 0=open, 255=full squint                     |
| +3      | Sclera R       | 0-255  | Direct                                       |
| +4      | Sclera G       | 0-255  | Direct                                       |
| +5      | Sclera B       | 0-255  | Direct                                       |
| +6      | Iris R         | 0-255  | Direct                                       |
| +7      | Iris G         | 0-255  | Direct                                       |
| +8      | Iris B         | 0-255  | Direct                                       |
| +9      | Auto Blink     | 0-1    | 0=off, 1=on                                  |
| +10     | Blink Speed    | 0-255  | 0=fast, 255=30s interval                      |
| +11     | Eye Mode       | 0-1    | 0=normal, 1=sprite animation                 |
| +12     | Sprite Index   | 0-255  | Sprite ID to trigger                         |

### Servo Block (offset +13 to +28)

16 channels mapped 1:1 to PCA9685 servo channels 0-15.

| Channel | Function | Range | Notes                          |
|---------|----------|-------|--------------------------------|
| +13     | Servo 0  | 0-255 | Mapped to configured limits    |
| +14     | Servo 1  | 0-255 | Mapped to configured limits    |
| ...     | ...      | ...   | ...                            |
| +28     | Servo 15 | 0-255 | Mapped to configured limits    |

Servo labels and limits are configured via telnet. DMX values 0-255 are mapped to each servo's configured lower/upper pulse limits.

### Config Channels (fixed, not offset-relative)

| Channel | Function  | Range | Notes                                          |
|---------|-----------|-------|-------------------------------------------------|
| 512     | WiFi Mode | 1-16  | Set to board ID + 1 to trigger WiFi config (e.g. 1=board 0) |

## Telnet Commands

Telnet runs on port 23, only after WiFi is active. Enable WiFi via the boot button, DMX channel 512, or debug mode (see Boot Behavior). WiFiManager auto-connects to a saved network, or opens a config portal AP named `skeleton-board-<id>` (password: `skeleton`).

### General

```
help                    show command list
status                  board id, wifi, dmx state
id                      print board id
led <r> <g> <b>         set status led (0-255)
debug                   show debug mode state
debug on                enable debug mode (persists, boots to WiFi)
debug off               disable debug mode (boots to DMX)
restart                 reboot device
```

### DMX

```
dmx                     show mapped eye/servo dmx values
dmx raw                 dump non-zero raw dmx channels
dmx all                 full dmx table (1-512)
dmx offset [val]        show or set dmx offset (1-512)
```

### Servo

```
servo                   show all servo config and state
servo <ch>              show servo config (ch 0-15)
servo <ch> move <pos>   move servo manually (102-512, overrides DMX)
servo <ch> auto         resume DMX control
servo <ch> lower <val>  set lower limit (102-512)
servo <ch> upper <val>  set upper limit (102-512)
servo <ch> center <val> set center (102-512)
servo <ch> smooth <val> set smoothing (0-255, 0=instant)
servo <ch> enable       enable servo
servo <ch> disable      disable servo
servo <ch> invert <on|off>  flip DMX direction
servo <ch> label <str>  set label (max 15 chars)
servo <ch> save         save config to flash
servo all on            enable all servo outputs (OE LOW)
servo all off           disable all servo outputs (OE HIGH)
```

### Eye

```
eye                     show eye status
eye look <x> <y>        move eye (-100 to 100)
eye jump <x> <y>        jump eye (-100 to 100)
eye idle                eye idle animation
eye blink [ms]          blink (default 200ms)
eye squint <0-255>      set squint level
eye sclera <r> <g> <b>  set sclera color (0-255)
eye iris <r> <g> <b>    set iris color (0-255)
eye autoblink <on|off>  toggle auto blink
eye blinkspeed <ms>     set auto blink interval
eye smooth <0-255>      set eye smoothing
eye mode <0|1>          set eye mode (0=normal,1=sprite)
eye spriteix <id>       set sprite index
eye reset               reset eye board
eye wifi ssid <ssid>    set eye board wifi ssid
eye wifi pass <pass>    set eye board wifi password
eye wifi connect        connect eye to wifi (stored creds)
eye wifi forget         clear eye wifi credentials
eye wifi status         show eye wifi status
```

### Envelope Detection

```
envelope                show envelope config and live amplitude
envelope enable         enable envelope->servo control
envelope disable        disable envelope->servo control
envelope servo <ch>     set target servo (0-15)
envelope min <pulse>    servo position at zero amplitude
envelope max <pulse>    servo position at max amplitude
envelope invert <on|off> flip amplitude direction
envelope gain <0-255>   amplitude gain (64=1.0x, 255=4x)
envelope smooth <0-255> amplitude smoothing (0=instant)
envelope interval <ms>  update interval (default 20)
envelope baseline       recalibrate DC offset (be quiet!)
envelope test           live amplitude readout (3s)
envelope calibrate      watch amplitude range (5s, sets min/max)
envelope monitor        live bar graph (10s)
```

### I2C

```
i2c scan                scan i2c bus for devices
i2c scan v              scan with verbose error codes
i2c scanretry [n]       scan n times, 1s apart
i2c read <addr> <reg> [len]  read register(s)
i2c write <addr> <reg> <val> [val2...]  write register(s)
```

## Building

```bash
pio run                          # build
pio run -t upload                # upload via USB
make ota-deploy ESP_IP=x.x.x.x  # upload via OTA
pio device monitor               # serial monitor
```

## Boot Behavior

- WiFi is off by default (saves power)
- Press the boot button (IO0) to start WiFi (unless already active)
- Set DMX channel 512 to board ID + 1 to trigger WiFi remotely (requires a live DMX frame)
- `debug on` persists — the board boots straight to WiFi on every power-up
- OTA updates available when WiFi is connected
- All servo channels are disabled by default — enable per-channel via `servo <ch> enable` (OE is asserted at boot, but no channel is driven until enabled)
