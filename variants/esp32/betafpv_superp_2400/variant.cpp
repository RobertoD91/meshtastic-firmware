#include "variant.h"
#include "Arduino.h"

// The BetaFPV SuperP is a True Diversity receiver: a second SX1280 + AT2401C is
// wired to the same SPI bus as the primary radio (shared SCK/MISO/MOSI/RESET).
// Meshtastic only drives radio 1. If the second radio's NSS is left floating it
// can drive the shared MISO line and make the primary SX1280 undetectable
// (RADIOLIB_ERR_CHIP_NOT_FOUND -> firmware never auto-selects the LORA_24 region
// and keeps prompting for one). Park the second radio before the LoRa stack
// initializes.
void earlyInitVariant()
{
    pinMode(SX128X_CS_2, OUTPUT);
    digitalWrite(SX128X_CS_2, HIGH); // hard-deselect the unused second SX1280

    pinMode(SX128X_TXEN_2, OUTPUT);
    digitalWrite(SX128X_TXEN_2, LOW); // keep the second AT2401C PA disabled
}
