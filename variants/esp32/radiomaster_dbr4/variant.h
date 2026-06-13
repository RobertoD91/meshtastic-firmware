/*
 * RadioMaster DBR4 - ESP32 ExpressLRS receiver with dual Semtech LR1121 radios.
 *
 * Pin mapping derived from ExpressLRS target:
 * romhack DBR4 Generic LR1121 True Diversity.json
 */

#define HAS_SCREEN 0
#define HAS_GPS 0
#undef GPS_RX_PIN
#undef GPS_TX_PIN

// SPI bus shared by both LR1121 radios.
#define LORA_SCK 25
#define LORA_MISO 33
#define LORA_MOSI 32

// Primary LR1121 radio.
#define USE_LR1121
#define LR1121_SPI_SCK_PIN LORA_SCK
#define LR1121_SPI_MISO_PIN LORA_MISO
#define LR1121_SPI_MOSI_PIN LORA_MOSI
#define LR1121_SPI_NSS_PIN 27
#define LR1121_IRQ_PIN 37
#define LR1121_BUSY_PIN 36
#define LR1121_NRESET_PIN 26

// Secondary LR1121 radio, parked off the shared SPI bus at boot.
#define LR1121_2_NSS_PIN 13
#define LR1121_2_NRESET_PIN 21
#define LR1121_2_BUSY_PIN 39
#define LR1121_2_IRQ_PIN 34

#define TCXO_OPTIONAL

// ExpressLRS uses low internal LR1121 power in dual-radio mode.
#define LR1110_MAX_POWER 3
#define LR1120_MAX_POWER 3

// WS2812 RGB status LED (GRB order).
#define HAS_NEOPIXEL
#define NEOPIXEL_COUNT 1
#define NEOPIXEL_DATA 22
#define NEOPIXEL_TYPE (NEO_GRB + NEO_KHZ800)

// Boot button.
#define BUTTON_PIN 0
#define BUTTON_NEED_PULLUP

#undef EXT_NOTIFY_OUT
