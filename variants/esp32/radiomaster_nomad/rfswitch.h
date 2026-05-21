#include "RadioLib.h"

/*
 * RF switch / antenna-switch routing for the RadioMaster Nomad's primary
 * LR1121. The LR1121's DIO5-DIO8 drive the module's external T/R and band
 * switches; the values below are verified against the ExpressLRS LR1121
 * driver (LR1121Driver::SetDioAsRfSwitch, default configuration):
 *
 *   enable = DIO5 | DIO6 | DIO7 | DIO8
 *   DIO5 = RXEN 2.4 GHz   DIO6 = TXEN 2.4 GHz
 *   DIO7 = RXEN sub-GHz   DIO8 = TXEN sub-GHz
 *
 * ExpressLRS per-mode masks: STBY 0x00, RX 0x04 (DIO7), TX/TX_HP 0x08 (DIO8),
 * TX_HF 0x02 (DIO6), GNSS 0x00, WIFI 0x01 (DIO5).
 */

static const uint32_t rfswitch_dio_pins[] = {RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6, RADIOLIB_LR11X0_DIO7,
                                             RADIOLIB_LR11X0_DIO8, RADIOLIB_NC};

static const Module::RfSwitchMode_t rfswitch_table[] = {
    // mode                 DIO5  DIO6  DIO7  DIO8
    {LR11x0::MODE_STBY, {LOW, LOW, LOW, LOW}},
    {LR11x0::MODE_RX, {LOW, LOW, HIGH, LOW}},
    {LR11x0::MODE_TX, {LOW, LOW, LOW, HIGH}},
    {LR11x0::MODE_TX_HP, {LOW, LOW, LOW, HIGH}},
    {LR11x0::MODE_TX_HF, {LOW, HIGH, LOW, LOW}},
    {LR11x0::MODE_GNSS, {LOW, LOW, LOW, LOW}},
    {LR11x0::MODE_WIFI, {HIGH, LOW, LOW, LOW}},
    END_OF_MODE_TABLE,
};
