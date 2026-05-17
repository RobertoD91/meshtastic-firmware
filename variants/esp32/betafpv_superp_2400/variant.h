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

// SX1280 2.4 GHz LoRa (primary radio only — second radio NSS=27/DIO1=34/BUSY=39 is
// physically present for True Diversity but unsupported by Meshtastic firmware; wired
// identically to radio 1 on the same SPI bus and left unselected at all times)
#define USE_SX1280
#define LORA_CS 26
#define LORA_RESET 25
#define SX128X_CS LORA_CS
#define SX128X_DIO1 38
#define SX128X_BUSY 37
#define SX128X_RESET LORA_RESET
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
