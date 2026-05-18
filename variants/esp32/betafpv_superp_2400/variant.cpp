#include "configuration.h"
#include "Arduino.h"

// The BetaFPV SuperP is a True Diversity receiver: a second SX1280 + AT2401C is
// wired to the same SPI bus as the primary radio (shared SCK/MISO/MOSI/RESET).
// Meshtastic only drives radio 1, so the second radio is parked here before the
// LoRa stack initializes. The captured pin state is logged later (Serial is not
// up yet in earlyInitVariant) to confirm this code path actually executed.
static volatile bool superp_earlyInitRan = false;
static volatile int superp_cs2Readback = -1;
static volatile int superp_busyAtBoot = -1;

void earlyInitVariant()
{
    pinMode(SX128X_CS_2, OUTPUT);
    digitalWrite(SX128X_CS_2, HIGH); // hard-deselect the unused second SX1280

    pinMode(SX128X_TXEN_2, OUTPUT);
    digitalWrite(SX128X_TXEN_2, LOW); // keep the second AT2401C PA disabled

    pinMode(SX128X_BUSY, INPUT);

    superp_cs2Readback = digitalRead(SX128X_CS_2);
    superp_busyAtBoot = digitalRead(SX128X_BUSY);
    superp_earlyInitRan = true;
}

void lateInitVariant()
{
    LOG_INFO("[SuperP] earlyInitVariant ran=%d, NSS2(GPIO%d) readback=%d, BUSY(GPIO%d) at boot=%d",
             superp_earlyInitRan ? 1 : 0, SX128X_CS_2, superp_cs2Readback, SX128X_BUSY, superp_busyAtBoot);
}
