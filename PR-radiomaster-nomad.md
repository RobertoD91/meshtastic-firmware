# PR: Add RadioMaster Nomad variant (ESP32 + LR1121)

## Summary

New board variant for the **RadioMaster Nomad** — a dual-band (sub-GHz +
2.4 GHz) ExpressLRS TX module: an ESP32 with two Semtech LR1121 transceivers,
an external RF power amplifier, a cooling fan and an addressable RGB LED.

Meshtastic drives the **primary LR1121 on the sub-GHz band**. Sub-GHz TX and
RX of mesh messages is confirmed working on hardware.

## Hardware (from the ExpressLRS target)

Pin map from `ExpressLRS/Targets` → `TX/Radiomaster Nomad.json`:

| Function | GPIO | Notes |
| --- | --- | --- |
| SPI SCK / MOSI / MISO | 25 / 32 / 33 | shared by both radios |
| Radio 1 NSS / RST / BUSY / DIO1 | 27 / 15 / 36 / 37 | used by Meshtastic |
| Radio 2 NSS / RST / BUSY / DIO1 | 13 / 21 / 39 / 34 | parked (unused) |
| `power_apc2` | 26 | ESP32 DAC — external PA bias |
| Fan enable | 2 | on/off |
| RGB LED (WS2812) | 22 | 2 LEDs |
| Buttons | 14 / 12 | |

## What the variant does

- `variant.h` / `variant.cpp` / `platformio.ini` / `rfswitch.h`
- Primary LR1121 wired up (`USE_LR1121`), TCXO optional (RadioLib falls back
  to XTAL if 1.6 V on DIO3 fails).
- `earlyInitVariant()` runs before radio detection and:
  - starts the fan and keeps it on unconditionally (`RF95_FAN_ALWAYS_ON`) —
    it also cools the PA, so it must run even if the radio is never found;
  - parks the **secondary LR1121** — held in reset and deselected from the
    shared SPI bus — since Meshtastic only drives one radio;
  - drives the PA APC line (GPIO26) to 0 V so the external PA is off.
- I2C is excluded: the board has no I2C peripherals and scanning the bus hung
  boot. See the companion PR for the matching build fix.
- RGB status LED and the user button are mapped.

## RadioLib change (needs upstreaming)

The LR1121 on the Nomad reports **transceiver-firmware id `0xF3`**, which
RadioLib's `LR11x0` driver does not recognise — `begin()` fails the version
check and the radio never initialises. It is currently patched via a
pre-build script that edits the RadioLib source in place.

**This must be upstreamed to RadioLib** (accept `0xF3` in the LR1121
firmware-version check) and the pre-build hack removed. Until then the patch
script is a hard dependency of this variant.

## External PA — `power_apc2` (GPIO26)

The LR1121 feeds an external PA whose gain/bias is set by an analog voltage on
GPIO26 (ESP32 DAC). Commit `07b335e` ("google da verificare") added dynamic
bias in `src/mesh/LR11x0Interface.cpp`, guarded by `#ifdef NOMAD_PA_APC_PIN`:
DAC `120` while transmitting, `0` in RX/standby.

**Assessment:** the *approach* is correct — the PA should only be biased
during TX. But the values are **not calibrated**. ExpressLRS pairs DAC `120`
with a chip output of −17…−3 dBm and only uses DAC `95` for its +5 dBm / 1 W
tier. The current code couples DAC `120` with the chip at up to +5 dBm
(`LR1110_MAX_POWER`/`LR1120_MAX_POWER = 5`) — not an ExpressLRS operating
point. Radiated power is therefore unpredictable and the PA may run in
compression.

**Recommendation:** before this lands, decide one of:
1. Ship with the PA off (sub-GHz already works PA-off) and treat PA bias as a
   follow-up — safest.
2. Implement the real ExpressLRS power-index → (chip dBm, DAC value) table.

Either way it needs RF-power measurement on hardware. Putting Nomad-specific
DAC code in the shared `LR11x0Interface.cpp` is also a wart; a variant hook
would be cleaner.

## RF switch

The ExpressLRS default configuration drives LR1121 `DIO5–DIO8` as the antenna
switch. An attempt to replicate that table in RadioLib was made and then
**reverted**: the Nomad transmits and receives on the sub-GHz band with the
DIO RF switch left unconfigured, so it is not required for sub-GHz operation.
`LR11X0_DIO_AS_RF_SWITCH` stays disabled. (It may matter for the 2.4 GHz path
or the external PA routing — unverified.)

## Status / known limitations

- **Sub-GHz (868/900 MHz): working** — TX and RX of mesh messages confirmed.
- **2.4 GHz (LR1120 path): errors, not tested.**
- TX power is **not calibrated** against the external PA (see above).
- The RadioLib `0xF3` patch is applied by a pre-build script — not yet upstream.
- **Not build-verified in this environment** (PlatformIO platform download is
  network-blocked). Needs `pio run -e radiomaster_nomad` locally.

## Files

- `variants/esp32/radiomaster_nomad/variant.h`
- `variants/esp32/radiomaster_nomad/variant.cpp`
- `variants/esp32/radiomaster_nomad/platformio.ini`
- `variants/esp32/radiomaster_nomad/rfswitch.h` (present but unused)
- `src/mesh/LR11x0Interface.cpp` — `#ifdef NOMAD_PA_APC_PIN` PA-bias hook
- hardware model `RADIOMASTER_NOMAD = 135` (already in the protobuf enum)

## Not for the PR

`analisis_nomad.md` and `todo.txt` are working notes and should not be
committed to the variant PR.
