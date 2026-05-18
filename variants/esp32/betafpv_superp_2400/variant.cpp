#include "configuration.h"
#include "Arduino.h"

// The BetaFPV SuperP is a True Diversity receiver: a second SX1280 + AT2401C is
// wired to the same SPI bus as the primary radio, and BOTH SX1280s share the
// reset line (LORA_RESET / GPIO25). On a cold boot the primary radio is detected
// fine, but Meshtastic's first boot has region UNSET, detects the 2.4 GHz-only
// chip, forces region LORA_24 and performs a SOFT reboot. On that warm reboot
// the SX1280 is never power-cycled and RadioLib's short reset pulse on the
// shared line is not enough to bring it back to a clean state, so it answers
// 0xFF and init fails with -2 ("No SX1280 radio"). Every power cycle repeats
// this, which is why the device appears to "always ask for the region".
//
// earlyInitVariant() runs on every boot (including warm reboots) before the
// LoRa stack, so we: deselect the unused second radio, then issue a long,
// well-delayed hardware reset of the SX1280 so RadioLib always starts from a
// freshly-reset chip.
static volatile bool superp_earlyInitRan = false;
static volatile int superp_cs2Readback = -1;
static volatile int superp_busyAfterReset = -1;

void earlyInitVariant()
{
    // Park the unused second SX1280 + its AT2401C off the shared SPI bus first.
    pinMode(SX128X_CS_2, OUTPUT);
    digitalWrite(SX128X_CS_2, HIGH);
    pinMode(SX128X_TXEN_2, OUTPUT);
    digitalWrite(SX128X_TXEN_2, LOW);

    // Keep the primary radio deselected while we reset it.
    pinMode(SX128X_CS, OUTPUT);
    digitalWrite(SX128X_CS, HIGH);

    pinMode(SX128X_BUSY, INPUT);

    // Robust SX1280 power-on-like reset (NRESET is active low). Generous timing
    // so a warm reboot gets a clean chip before RadioLib talks to it.
    pinMode(SX128X_RESET, OUTPUT);
    digitalWrite(SX128X_RESET, LOW);
    delay(20);
    digitalWrite(SX128X_RESET, HIGH);
    delay(20);

    superp_cs2Readback = digitalRead(SX128X_CS_2);
    superp_busyAfterReset = digitalRead(SX128X_BUSY);
    superp_earlyInitRan = true;
}

void lateInitVariant()
{
    LOG_INFO("[SuperP] earlyInitVariant ran=%d, NSS2(GPIO%d)=%d, BUSY(GPIO%d) after reset=%d",
             superp_earlyInitRan ? 1 : 0, SX128X_CS_2, superp_cs2Readback, SX128X_BUSY, superp_busyAfterReset);
}
