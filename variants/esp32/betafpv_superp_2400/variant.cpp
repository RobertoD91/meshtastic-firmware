#include "variant.h"
#include "Arduino.h"

// The BetaFPV SuperP is a True Diversity receiver: a second SX1280 + AT2401C is
// wired to the same SPI bus (shared SCK/MISO/MOSI) and the same RESET line as
// the primary radio. Meshtastic only drives radio 1, so park the unused second
// radio off the bus before the LoRa stack runs and give the primary chip a
// clean, well-delayed reset. RadioLib still owns SX128X_RESET (handing it
// RADIOLIB_NC was tested and made detection strictly worse).
//
// NOTE: with this the SX1280 is detected on a cold boot but RadioLib then loses
// the chip part-way through configuration; the radio is not yet usable under
// Meshtastic. See README.md in this folder for the full investigation.
void earlyInitVariant()
{
    pinMode(SX128X_CS_2, OUTPUT);
    digitalWrite(SX128X_CS_2, HIGH); // hard-deselect the unused second SX1280
    pinMode(SX128X_TXEN_2, OUTPUT);
    digitalWrite(SX128X_TXEN_2, LOW); // keep the second AT2401C PA disabled

    pinMode(SX128X_CS, OUTPUT);
    digitalWrite(SX128X_CS, HIGH); // primary radio deselected during reset

    pinMode(LORA_RESET, OUTPUT);
    digitalWrite(LORA_RESET, LOW);
    delay(20);
    digitalWrite(LORA_RESET, HIGH);
    delay(20);
}
