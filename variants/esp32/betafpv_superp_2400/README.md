# BetaFPV SuperP 2.4GHz RX — Meshtastic port status & investigation log

> Target env: `betafpv_superp_2400` — HardwareModel `BETAFPV_SUPERP_2400_RX` (136)

## TL;DR

The board boots, the ESP32 works, BLE/WiFi/phone app work, the RGB LED works —
**but the LoRa radio is not usable under Meshtastic yet.** RadioLib fails to
detect the SX1280 (`SX128x init result -2`, `No SX1280 radio`). The exact same
hardware works perfectly when flashed with ExpressLRS, so this is **not** a
hardware, wiring, power, or pin-mapping problem — it is a RadioLib ↔ board
SPI/BUSY timing integration issue. See the investigation below.

This document records everything found and tried so the next person does not
repeat the dead ends.

## Hardware

- BetaFPV SuperP 14CH 2.4GHz diversity receiver — https://betafpv.com/products/superp-14ch-rx
- MCU: **ESP32-PICO-D4** (confirmed by esptool: `ESP32-PICO-D4 (revision v1.1)`)
- Radio: **2× SX1280** (true diversity) + **2× AT2401C** PA/LNA front-ends
- Meshtastic uses only radio 1; radio 2 shares SCK/MISO/MOSI and the RESET line.

### Pinout (verified against the official ExpressLRS layout)

The ExpressLRS target *"BETAFPV SuperP 14Ch 2.4GHz RX"* uses layout
`Generic 2400 True Diversity PA PWM 14.json`. Every Meshtastic pin matches it:

| Function        | Pin (GPIO) | Function          | Pin (GPIO) |
|-----------------|------------|-------------------|------------|
| SCK             | 33         | Radio1 NSS        | 26         |
| MISO            | 35         | Radio1 RESET      | 25 (shared)|
| MOSI            | 32         | Radio1 BUSY       | 37         |
| Radio1 DIO1     | 38         | Radio1 TXEN (PA)  | 14         |
| Radio2 NSS      | 27         | Radio2 DIO1       | 34         |
| Radio2 BUSY     | 39         | Radio2 TXEN/LNA   | 12         |
| I2C SDA / SCL   | 22 / 19    | WS2812 LED (GRB)  | 21         |

The pinout is **correct** and not the cause of the failure.

## Current status

| Subsystem            | State                                                        |
|----------------------|--------------------------------------------------------------|
| Build / flash        | OK (`pio run -e betafpv_superp_2400 -t upload`)              |
| ESP32 / FS / NVS     | OK                                                           |
| BLE / WiFi / phone   | OK                                                           |
| WS2812 status LED    | OK — fixed green (see "Status LED" below)                     |
| **LoRa SX1280**      | **NOT WORKING** — `SX128x init result -2`, `No SX1280 radio` |

Because the radio is never detected, Meshtastic records
`CriticalErrorCode_NO_RADIO` (3) and the device cannot mesh. With region UNSET
it briefly reaches the 906 MHz path and returns `-12` (INVALID_FREQUENCY, the
SX1280 is 2.4 GHz only); Meshtastic then auto-sets region `LORA_24` and reboots,
and from then on it never gets past `-2`. This is why the device "always asks
for the region".

## Symptom analysis (RadioLib SPI debug)

With `RADIOLIB_DEBUG_SPI` enabled we can see the exact bus traffic:

- The SX1280 **answers short commands**: after `CMDW 80` (SetStandby) it
  returns status byte `0x43`. So SPI, MISO, NSS and the chip fundamentally work.
- The **multi-byte version-register read** (`CMDR 1901F0`,
  `RADIOLIB_SX128X_REG_VERSION_STRING`) comes back **byte-shifted by a varying
  amount, padded with leading `0xFF`**. The correct bytes are present but slip
  to a different offset on every attempt, e.g.:
  - cold first boot: `... 53 58 31 32 38 30 20 56 33 42 20 41 39 42 37 00` → `SX1280 V3B A9B7` (correct)
  - later boots: `... FF FF FF FF 33 42 20 41 39 42 37 00` → tail `3B A9B7` shifted; another try `... 39 42 37 00`; etc.

Interpretation: the chip is **still BUSY** when RadioLib clocks the register
read, so it emits a variable number of leading dummy/FF bytes before the real
data, and RadioLib's fixed-length read window then captures a shifted/truncated
string → "SX128x not found" → `-2`. ExpressLRS works because its bespoke SX1280
driver waits on BUSY meticulously; RadioLib's generic driver does not, on this
board.

### Refinement after instrumenting the BUSY pin

A later diagnostic build tightly sampled `GPIO37` right after the reset and
logged: `BUSY(GPIO37) everHigh=1 highForUs=1395`. So **the BUSY pin works
correctly** — it goes HIGH for ~1.4 ms after reset, exactly as a healthy
SX1280 should. On that cold boot the version string was also read **perfectly**
(`SX1280 V3B A9B7`, "Found SX128x"); then RadioLib issued config commands and
the chip went unresponsive after a few (`CMDW 88 → SO FF FF`), ending in `-2`.

So, more precisely: the BUSY pin and reset are fine and detection succeeds on a
cold boot, but **the chip drops off the bus part-way through RadioLib's SX128x
configuration sequence**, intermittently. The earlier "garbled version string"
is the same root cause seen on warm/repeat boots. It is RadioLib's command/BUSY
sequencing on this specific board — not the pin, not detection per se.

## What has been ruled out

| Hypothesis                         | How it was ruled out |
|------------------------------------|----------------------|
| Wrong pins                         | 100% match to official ELRS layout; ELRS works on this unit |
| Power / brownout                   | Reproduced on a 2S LiPo with **no** brownout; still `-2` |
| Second SX1280 contending the bus   | `earlyInitVariant` deselects radio 2 (`NSS2/GPIO27=1`, verified); failures are `0xFF` (idle), not `0x00` (contention) |
| SPI clock too fast                 | Lowered to 1 MHz via `LORA_SPI_FREQUENCY` — identical garbage |
| RadioLib reset too short / warm-reboot | Long ELRS-style reset + active BUSY wait added — still `-2` |
| Letting us own the reset (RADIOLIB_NC) | **Made it worse** — chip then returns all `0xFF` (RadioLib's reset is required); reverted |
| BUSY pin (GPIO37) not wired/readable    | **Disproved** — measured HIGH for ~1395 µs after reset; the pin works |

## Diagnosis (current best understanding)

This is a **RadioLib SX128x ↔ this specific diversity board** integration
problem at the SPI/BUSY-timing level. RadioLib begins the version-register read
before the SX1280 has cleared BUSY. The open question being measured now: does
`GPIO37` (the BUSY pin) actually reflect the chip's BUSY state in Meshtastic's
runtime, or is RadioLib effectively never waiting on it?

## Changes made on this branch

### Solid, keepable fixes
- **`platformio.ini` (root)**: `default_envs` switched to the newline-list form
  (space-separated is parsed by PlatformIO as one env name and fails).
- **Status LED**: `ENABLE_AMBIENTLIGHTING` for this target + a new generic,
  macro-guarded fixed-colour override in `src/AmbientLightingThread.h`
  (`AMBIENTLIGHTING_RED/GREEN/BLUE/CURRENT`). No effect on other boards that
  use `ENABLE_AMBIENTLIGHTING` (they don't define the macros). The SuperP shows
  a steady green "alive" LED.
- **Second radio parking**: `earlyInitVariant()` drives radio 2 `NSS` (GPIO27)
  high and its PA control (GPIO12) low so the unused SX1280 cannot disturb the
  shared SPI bus. This is correct hygiene for a true-diversity board regardless.
- **`LORA_SPI_FREQUENCY`** macro added to `src/mesh/RadioInterface.cpp`
  (default 4 MHz, unchanged for every other board) so the LoRa SPI clock is
  per-target tunable.

### Diagnostics (removed from this branch — how to re-enable)
The temporary instrumentation has been stripped so the branch is clean. To
continue the radio investigation, re-add to the target `platformio.ini`
`build_flags`:

```
-DRADIOLIB_DEBUG_BASIC=1
-DRADIOLIB_DEBUG_SPI=1
-DRADIOLIB_DEBUG_PORT=Serial
```

That dumps every SX1280 SPI transaction (`RLB_SPI`/`RLB_DBG`) at boot, which is
how all of the above was diagnosed. A BUSY-pin probe can be re-added in
`earlyInitVariant()`/`lateInitVariant()` if needed (see git history of this
branch for the exact snippet).

## How to reproduce / collect logs

```bash
git checkout claude/fix-betafpv-target-tlBdb
pio run -e betafpv_superp_2400 -t upload && pio run -e betafpv_superp_2400 -t monitor
```

Serial is UART0 (GPIO1 TX / GPIO3 RX) at 115200 — solder a 3.3 V USB-UART to
the pads (the RX has no USB). Capture from the `SX128xInterface(cs=26 ...)` line
through `SX128x init result` and include the `RLB_*` and `[SuperP]` lines.

## Remaining options

1. **Patch RadioLib's SX128x driver** to wait on BUSY around the register read
   (mimic ExpressLRS). RadioLib is a zip `lib_deps` dependency, so this needs a
   post-install patch script or a RadioLib fork, plus hardware-in-the-loop
   iteration.
2. **Ship the target without a working Meshtastic radio for now**: strip the
   temporary diagnostics, keep the solid fixes above, and treat the SX1280
   detection as a known limitation (BLE/WiFi/LED usable).

**This branch implements option 2**: diagnostics removed, solid fixes kept,
SX1280-under-Meshtastic documented as a known limitation. Re-enable the SPI
debug flags (above) to resume option 1.
