/*
 * RadioMaster Nomad - ESP32 TX module with dual Semtech LR1121 radios.
 * https://www.radiomasterrc.com/products/nomad-dual-1-watt-gemini-xrossband-expresslrs-module
 *
 * Pin mapping derived from ExpressLRS Targets:
 * https://github.com/ExpressLRS/Targets/blob/master/TX/Radiomaster%20Nomad.json
 *
 * Safe first-port assumptions:
 * - only the primary LR1121 is used by Meshtastic
 * - the secondary LR1121 is held in reset and deselected from the shared SPI bus
 * - the external PA is kept off (APC bias line driven low)
 * - LR1121 transmit power is clamped low so it cannot overdrive the PA
 * - guessed RF switch tables stay disabled until measured/verified
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

  ExpressLRS Nomad target (radio 1):
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
  Secondary LR1121 radio. Meshtastic only ever drives the primary radio.
  earlyInitVariant() (variant.cpp) keeps this chip held in reset and
  deselected from the shared SPI bus so it stays completely silent.

  ExpressLRS Nomad target (radio 2):
    radio_nss_2  = 13
    radio_busy_2 = 39
    radio_dio1_2 = 34
    radio_rst_2  = 21
*/
#define LR1121_2_NSS_PIN 13
#define LR1121_2_NRESET_PIN 21
#define LR1121_2_BUSY_PIN 39
#define LR1121_2_IRQ_PIN 34

/*
  The ExpressLRS target does not document a TCXO voltage. Keep TCXO optional:
  RadioLib tries 1.6 V on DIO3 first and transparently falls back to XTAL mode.
*/
#define TCXO_OPTIONAL

/*
  Safe bring-up power clamp. The LR1121 feeds an external PA, so its own
  transmit power is clamped to a low level until the RF chain is validated.
  LR1110_MAX_POWER is the firmware's generic LR11x0 sub-GHz clamp;
  LR1120_MAX_POWER is the 2.4 GHz clamp. Receive is unaffected.
  Raise these deliberately once the hardware has been verified.
*/
#define LR1110_MAX_POWER 5
#define LR1120_MAX_POWER 5

/*
  Do not enable a guessed LR11x0 RF switch table yet.
  The ExpressLRS target does not expose the DIO5/DIO6 RF switch truth table.
*/
// #define LR11X0_DIO_AS_RF_SWITCH

/*
  GPIO26 is ExpressLRS power_apc2: the analog gain/bias control for the
  on-board RF power amplifier. earlyInitVariant() drives it LOW so the
  external PA stays off during bring-up. Do not raise it until the RF
  path has been verified.
*/
#define NOMAD_PA_APC_PIN 26

/*
  Built-in fan, controlled by GPIO2.
  The fan also cools the PA, so it must always run:
  - earlyInitVariant() starts it at the very beginning of boot, before
    radio detection, so it spins even if the LR1121 is never found.
  - RF95_FAN_ALWAYS_ON keeps it on unconditionally (ignores the
    pa_fan_disabled config option).
*/
#define RF95_FAN_EN 2
#define RF95_FAN_ALWAYS_ON

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
