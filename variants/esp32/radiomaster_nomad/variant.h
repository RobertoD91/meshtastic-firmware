/*
 * RadioMaster Nomad - ESP32 TX module with dual Semtech LR1121 radios.
 * https://www.radiomasterrc.com/products/nomad-dual-1-watt-gemini-xrossband-expresslrs-module
 *
 * Pin mapping derived from ExpressLRS Targets:
 * https://github.com/ExpressLRS/Targets/blob/master/TX/Radiomaster%20Nomad.json
 *
 * Safe first-port assumptions:
 * - only the primary LR1121 is used by Meshtastic
 * - the secondary LR1121 is left untouched
 * - external PA/APC control on GPIO26 is left untouched
 * - guessed RF switch tables are disabled until measured/verified
 */

#define HAS_GPS 0
#undef GPS_RX_PIN
#undef GPS_TX_PIN

/*
  SPI bus shared by both LR1121 radios.
*/
#define LORA_SCK 25
#define LORA_MISO 33
#define LORA_MOSI 32

/*
  Primary LR1121 radio pin connections.

  ExpressLRS Nomad target:
    radio_nss  = 27
    radio_busy = 36
    radio_dio1 = 37
    radio_rst  = 15
*/
#define USE_LR1121
#define LR1121_SPI_SCK_PIN LORA_SCK
#define LR1121_SPI_MISO_PIN LORA_MISO
#define LR1121_SPI_MOSI_PIN LORA_MOSI
#define LR1121_SPI_NSS_PIN 27
#define LR1121_IRQ_PIN 37
#define LR1121_BUSY_PIN 36
#define LR1121_NRESET_PIN 15

/*
  The ExpressLRS target does not explicitly document a TCXO voltage.
  Keep TCXO optional for the first smoke tests instead of forcing DIO3 to 1.8 V.
*/
#define TCXO_OPTIONAL

/*
  Do not enable a guessed LR11x0 RF switch table yet.
  The ExpressLRS target does not expose the DIO5/DIO6 RF switch truth table.
*/
// #define LR11X0_DIO_AS_RF_SWITCH

/*
  GPIO26 is ExpressLRS power_apc2. It is likely an analog PA/APC control,
  not a digital PA enable. Do not use it as PA_ENABLE/TXEN/RXEN and do not
  digitalWrite/dacWrite it in this safe first-port variant.
*/

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
