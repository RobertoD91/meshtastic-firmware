#include "variant.h"
#include "Arduino.h"

/*
 * RadioMaster Nomad early hardware bring-up.
 *
 * Runs from main.cpp setup() before the LoRa stack (and before radio
 * detection), so anything that must hold regardless of radio init success
 * belongs here.
 */
void earlyInitVariant()
{
    // 1. Fan ON immediately and unconditionally. The fan also cools the PA, so
    //    it must run even if the LR1121 is never detected. GPIO2 is an ESP32
    //    boot strapping pin; it is only safe to drive it once boot is done,
    //    which is the case by the time earlyInitVariant() runs.
    pinMode(RF95_FAN_EN, OUTPUT);
    digitalWrite(RF95_FAN_EN, HIGH);

    // 2. Keep the external PA off during bring-up by driving its APC bias
    //    line low. This guarantees the amplifier cannot be overdriven before
    //    the RF path has been verified.
    pinMode(NOMAD_PA_APC_PIN, OUTPUT);
    digitalWrite(NOMAD_PA_APC_PIN, LOW);

    // 3. Park the unused secondary LR1121: deselect it from the shared SPI bus
    //    and hold it in reset so it stays completely silent.
    pinMode(LR1121_2_NSS_PIN, OUTPUT);
    digitalWrite(LR1121_2_NSS_PIN, HIGH); // NSS idle high = deselected
    pinMode(LR1121_2_NRESET_PIN, OUTPUT);
    digitalWrite(LR1121_2_NRESET_PIN, LOW); // NRESET low = held in reset (off)

    // 4. Deselect the primary LR1121 too until RadioLib takes over its NSS,
    //    so neither chip drives MISO while the SPI bus is being set up.
    pinMode(LR1121_SPI_NSS_PIN, OUTPUT);
    digitalWrite(LR1121_SPI_NSS_PIN, HIGH);
}
