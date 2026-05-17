// https://betafpv.com/products/superp-14ch-rx
// BetaFPV SuperP 2.4GHz 14ch Receiver
// ESP32 Pico D4 + SX1280/SX1281 (True Diversity, primary radio used) + AT2401C PA
// ExpressLRS target: Generic 2400 True Diversity PA PWM 14

// No onboard screen
#define HAS_SCREEN 0

// No GPS (RX module)
#undef GPS_RX_PIN
#undef GPS_TX_PIN

// I2C pads available for expansion (no onboard display)
#define I2C_SDA 22
#define I2C_SCL 19

// SPI bus — non-default pins used by the radio
#define LORA_SCK 33
#define LORA_MISO 35
#define LORA_MOSI 32

// SX1280 2.4 GHz LoRa (primary radio only). The board is True Diversity with a
// second SX1280 on the SAME SPI bus (shared SCK/MISO/MOSI/RESET). Meshtastic only
// drives radio 1, so the second radio MUST be explicitly deselected at boot —
// otherwise its floating NSS lets it contend on the shared MISO line and the
// primary radio fails detection (RADIOLIB_ERR_CHIP_NOT_FOUND). Deselection is
// done in earlyInitVariant() (variant.cpp) using the *_2 pins below.
#define USE_SX1280
#define LORA_CS 26
#define LORA_RESET 25
#define SX128X_CS LORA_CS
#define SX128X_DIO1 38
#define SX128X_BUSY 37
#define SX128X_RESET LORA_RESET

// Second (unused) SX1280 + AT2401C — parked off at boot, never selected
#define SX128X_CS_2 27
#define SX128X_DIO1_2 34
#define SX128X_BUSY_2 39
#define SX128X_TXEN_2 12
// AT2401C PA: TXEN=HIGH activates the PA for TX; when LOW the built-in LNA handles RX.
// No dedicated RXEN pin — the AT2401C LNA is always on when TXEN is de-asserted.
#define SX128X_TXEN 14
#define SX128X_MAX_POWER 3

// WS2812 RGB status LED (GRB order)
#define HAS_NEOPIXEL
#define NEOPIXEL_COUNT 1
#define NEOPIXEL_DATA 21
#define NEOPIXEL_TYPE (NEO_GRB + NEO_KHZ800)

#undef EXT_NOTIFY_OUT
