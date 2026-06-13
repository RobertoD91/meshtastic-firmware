#include "variant.h"
#include "Arduino.h"

void earlyInitVariant()
{
    pinMode(LR1121_2_NSS_PIN, OUTPUT);
    digitalWrite(LR1121_2_NSS_PIN, HIGH);
    pinMode(LR1121_2_NRESET_PIN, OUTPUT);
    digitalWrite(LR1121_2_NRESET_PIN, LOW);

    pinMode(LR1121_SPI_NSS_PIN, OUTPUT);
    digitalWrite(LR1121_SPI_NSS_PIN, HIGH);
}
