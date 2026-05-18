#include "configuration.h"
#include "Arduino.h"

// SPI debug showed the SX1280 answers short commands (status 0x43) but the
// version-register read comes back byte-shifted by a varying amount with
// leading 0xFF: RadioLib is reading before the chip has cleared BUSY. ExpressLRS
// works because it resets the chip and then actively waits for BUSY low. We do
// the same here and hand RadioLib RADIOLIB_NC for reset (see variant.h) so its
// own short reset pulse can't re-wedge the chip right before the version read.
static volatile bool superp_earlyInitRan = false;
static volatile int superp_cs2Readback = -1;
static volatile int superp_busyWaitMs = -1; // ms waited for BUSY low (-1 = not run)

void earlyInitVariant()
{
    // Park the unused second SX1280 + its AT2401C off the shared SPI bus.
    pinMode(SX128X_CS_2, OUTPUT);
    digitalWrite(SX128X_CS_2, HIGH);
    pinMode(SX128X_TXEN_2, OUTPUT);
    digitalWrite(SX128X_TXEN_2, LOW);

    // Keep the primary radio deselected while we reset it.
    pinMode(SX128X_CS, OUTPUT);
    digitalWrite(SX128X_CS, HIGH);

    pinMode(SX128X_BUSY, INPUT);

    // ELRS-style reset on the shared NRESET line (active low), generous timing.
    pinMode(LORA_RESET, OUTPUT);
    digitalWrite(LORA_RESET, LOW);
    delay(50);
    digitalWrite(LORA_RESET, HIGH);
    delay(50);

    // Actively wait for the SX1280 to clear BUSY (datasheet: BUSY high while the
    // chip boots its firmware after reset). Bounded so a dead pin can't hang us.
    int waited = 0;
    while (digitalRead(SX128X_BUSY) == HIGH && waited < 200) {
        delay(1);
        waited++;
    }
    delay(10); // extra settle margin before any SPI traffic

    superp_busyWaitMs = waited;
    superp_cs2Readback = digitalRead(SX128X_CS_2);
    superp_earlyInitRan = true;
}

void lateInitVariant()
{
    LOG_INFO("[SuperP] earlyInitVariant ran=%d, NSS2(GPIO%d)=%d, BUSY-low wait=%d ms",
             superp_earlyInitRan ? 1 : 0, SX128X_CS_2, superp_cs2Readback, superp_busyWaitMs);
}
