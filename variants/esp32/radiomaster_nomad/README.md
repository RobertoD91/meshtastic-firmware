# RadioMaster Nomad (ESP32 + LR1121)

Meshtastic board variant for the [RadioMaster Nomad](https://www.radiomasterrc.com/products/nomad-dual-1-watt-gemini-xrossband-expresslrs-module),
a dual-band ExpressLRS TX module: an ESP32 with two Semtech LR1121
transceivers, an external RF power amplifier, a cooling fan and a 2-LED
addressable RGB strip.

Treated as **private hardware** (`PRIVATE_HW`) — it is not an officially
registered Meshtastic hardware model.

## Scope

Meshtastic drives only the **primary LR1121 on the sub-GHz band**.
`earlyInitVariant()` (see `variant.cpp`) runs before radio detection and:

- starts the cooling fan (it also cools the PA, so it must run even if the
  radio is never found);
- parks the **secondary LR1121** — held in reset and deselected from the
  shared SPI bus — so it cannot corrupt transfers to the primary radio;
- drives the external PA bias line (APC, GPIO26) low so the PA stays off.

| Item | Status |
| --- | --- |
| Sub-GHz (868/900 MHz) TX/RX | reported working on hardware (earlier testing) |
| 2.4 GHz (LR1120 path) | not tested |
| External PA | kept off — TX power not calibrated against the PA |
| Second LR1121 | unused, parked |

## TCXO / reference clock

This variant does **not** configure a DIO3-powered TCXO: the LR1121 runs
from its crystal (no `TCXO_OPTIONAL`, no `LR11X0_DIO3_TCXO_VOLTAGE`).

This matches the ExpressLRS reference firmware — the ExpressLRS LR1121
driver issues no `SetTcxoMode`/DIO3 command and the `Radiomaster Nomad.json`
target exposes no TCXO field. If hardware testing ever shows the radio
needs a DIO3 TCXO, add `#define LR11X0_DIO3_TCXO_VOLTAGE 1.6` (or the
measured voltage) to `variant.h`; that needs no change outside this variant.

## Known upstream issues to report

**`TCXO_OPTIONAL` is SX126x-only in `src/main.cpp`.** The global
`tcxoVoltage` is declared as:

```c
#if defined(TCXO_OPTIONAL)
float tcxoVoltage = SX126X_DIO3_TCXO_VOLTAGE;
#endif
```

A non-SX126x board (LR11x0 / LR20x0) that defines `TCXO_OPTIONAL` fails to
compile, because `SX126X_DIO3_TCXO_VOLTAGE` is undefined. `LR11x0Interface`
and `LR20x0Interface` already handle `TCXO_OPTIONAL` with their own local
`tcxoVoltage`, so the global is only meaningful for SX126x. Suggested fix:
guard it with `&& defined(SX126X_DIO3_TCXO_VOLTAGE)`, or extend it to the
LR11x0/LR20x0 TCXO voltage macros. This variant sidesteps the bug by not
defining `TCXO_OPTIONAL`.

## RadioLib LR1121 0xF3 patch

`bin/platformio-pre.py` carries a pre-build patch: an LR1121 running its
transceiver firmware reports a GetVersion device id of `0xF3`, which
upstream RadioLib's `LR11x0::findChip` does not accept, so the chip is
never detected. The patch teaches `findChip` to also accept `0xF3`.
This should be upstreamed to RadioLib so the pre-build patch can be dropped.

## RF switch (DIO5-DIO8)

`LR11X0_DIO_AS_RF_SWITCH` is enabled; `rfswitch.h` drives the LR1121's
DIO5-DIO8 antenna / T-R switch. The table is verified against the
ExpressLRS LR1121 driver (`LR1121Driver::SetDioAsRfSwitch`), so RadioLib
issues the exact `SetDioAsRfSwitch` command ExpressLRS uses on this
hardware. Routing: `DIO5 = RXEN 2.4G`, `DIO6 = TXEN 2.4G`,
`DIO7 = RXEN sub-GHz`, `DIO8 = TXEN sub-GHz`.

This should be retested on hardware: sub-GHz TX/RX was previously
confirmed with the RF switch *unconfigured*, and an earlier attempt that
used a wrong (DIO5/DIO6-only) table was reverted. The table here is the
correct one, but a bench check is still warranted.
