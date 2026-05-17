/*
 * Radiomaster Nomad - ESP32 TX module with dual LR1110 radios.
 * https://www.radiomasterrc.com/products/nomad-elrs-rf-module
 *
 * Pin mapping derived from ExpressLRS Targets:
 * https://github.com/ExpressLRS/Targets/blob/master/TX/Radiomaster%20Nomad.json
 *
 * Only the first LR1110 (primary radio) is used for Meshtastic.
 */

#define HAS_GPS 0
#undef GPS_RX_PIN
#undef GPS_TX_PIN

/*
  SPI bus shared by both LR1110 radios.
*/
#define LORA_SCK 25
#define LORA_MISO 33
#define LORA_MOSI 32

/*
  LR1110 primary radio pin connections.
*/
#define USE_LR1110
#define LR1110_SPI_SCK_PIN LORA_SCK
#define LR1110_SPI_MISO_PIN LORA_MISO
#define LR1110_SPI_MOSI_PIN LORA_MOSI
#define LR1110_SPI_NSS_PIN 27
#define LR1110_IRQ_PIN 37
#define LR1110_BUSY_PIN 36
#define LR1110_NRESET_PIN 15
#define LR11X0_DIO3_TCXO_VOLTAGE 1.8
#define LR11X0_DIO_AS_RF_SWITCH

/*
  This module has a built-in fan controlled by GPIO2.
*/
#define RF95_FAN_EN 2

/*
  NeoPixel RGB LED (2 LEDs, GRB order).
*/
#define HAS_NEOPIXEL
#define NEOPIXEL_COUNT 2
#define NEOPIXEL_DATA 22
#define NEOPIXEL_TYPE (NEO_GRB + NEO_KHZ800)

/*
  Button pins.
*/
#define BUTTON_PIN 14
#define BUTTON_NEED_PULLUP

/*
  No external notification output.
*/
#undef EXT_NOTIFY_OUT
