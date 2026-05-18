#include "configuration.h"
#include "Arduino.h"

// Decisive diagnostic: does GPIO37 (SX128X_BUSY) actually reflect the SX1280
// BUSY line in Meshtastic's runtime? A healthy SX1280 holds BUSY HIGH for
// ~1 ms after NRESET is released while it boots its firmware. We pulse the
// shared reset and then sample GPIO37 tightly for a window, recording whether
// it was EVER high and how long until it went low. RadioLib still owns the
// reset that actually brings the chip up (RADIOLIB_NC made it worse); this
// only deselects the second radio and instruments the BUSY pin.
static volatile bool superp_earlyInitRan = false;
static volatile int superp_cs2Readback = -1;
static volatile int superp_busyEverHigh = -1; // 1 if GPIO37 ever read HIGH in the window
static volatile int superp_busyHighUs = -1;   // approx microseconds BUSY stayed high

void earlyInitVariant()
{
    pinMode(SX128X_CS_2, OUTPUT);
    digitalWrite(SX128X_CS_2, HIGH);
    pinMode(SX128X_TXEN_2, OUTPUT);
    digitalWrite(SX128X_TXEN_2, LOW);

    pinMode(SX128X_CS, OUTPUT);
    digitalWrite(SX128X_CS, HIGH);

    pinMode(SX128X_BUSY, INPUT);

    pinMode(LORA_RESET, OUTPUT);
    digitalWrite(LORA_RESET, LOW);
    delay(20);
    digitalWrite(LORA_RESET, HIGH);

    // Tight-sample GPIO37 for 5 ms right after the reset rising edge.
    bool everHigh = false;
    uint32_t firstHighUs = 0, lastHighUs = 0;
    uint32_t t0 = micros();
    while ((uint32_t)(micros() - t0) < 5000) {
        if (digitalRead(SX128X_BUSY) == HIGH) {
            uint32_t now = micros();
            if (!everHigh) {
                everHigh = true;
                firstHighUs = now;
            }
            lastHighUs = now;
        }
    }
    superp_busyEverHigh = everHigh ? 1 : 0;
    superp_busyHighUs = everHigh ? (int)(lastHighUs - firstHighUs) : 0;

    delay(30); // settle margin before the LoRa stack runs

    superp_cs2Readback = digitalRead(SX128X_CS_2);
    superp_earlyInitRan = true;
}

void lateInitVariant()
{
    LOG_INFO("[SuperP] earlyInit ran=%d NSS2(GPIO%d)=%d | BUSY(GPIO%d) everHigh=%d highForUs=%d",
             superp_earlyInitRan ? 1 : 0, SX128X_CS_2, superp_cs2Readback, SX128X_BUSY,
             superp_busyEverHigh, superp_busyHighUs);
}
