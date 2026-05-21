#include "RadioLib.h"

/*
  RF switch table for the RadioMaster Nomad LR1121.

  The Nomad has no custom RF-switch entry in its ExpressLRS target, so it uses
  the ExpressLRS default LR1121 RF-switch configuration: the antenna switch is
  driven by LR1121 DIO5..DIO8 (SetDioAsRfSwitch enable mask 0b00001111).

  ExpressLRS default per-mode truth table (bit0=DIO5 .. bit3=DIO8):
    RfSwStbyCfg  = 0b0000  -> all low
    RfSwRxCfg    = 0b0100  -> DIO7 high
    RfSwTxCfg    = 0b1000  -> DIO8 high
    RfSwTxHpCfg  = 0b1000  -> DIO8 high
    RfSwTxHfCfg  = 0b0010  -> DIO6 high
    RfSwWifiCfg  = 0b0001  -> DIO5 high
*/

static const uint32_t rfswitch_dio_pins[Module::RFSWITCH_MAX_PINS] = {
    RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6, RADIOLIB_LR11X0_DIO7, RADIOLIB_LR11X0_DIO8, RADIOLIB_NC};

static const Module::RfSwitchMode_t rfswitch_table[] = {
    // mode              DIO5  DIO6  DIO7  DIO8
    {LR11x0::MODE_STBY, {LOW, LOW, LOW, LOW}},   {LR11x0::MODE_RX, {LOW, LOW, HIGH, LOW}},
    {LR11x0::MODE_TX, {LOW, LOW, LOW, HIGH}},    {LR11x0::MODE_TX_HP, {LOW, LOW, LOW, HIGH}},
    {LR11x0::MODE_TX_HF, {LOW, HIGH, LOW, LOW}}, {LR11x0::MODE_GNSS, {LOW, LOW, LOW, LOW}},
    {LR11x0::MODE_WIFI, {HIGH, LOW, LOW, LOW}},  END_OF_MODE_TABLE,
};
