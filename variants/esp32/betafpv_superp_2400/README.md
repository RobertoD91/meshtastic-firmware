# BetaFPV SuperP 2.4GHz RX — Meshtastic port status & investigation log

> Target env: `betafpv_superp_2400` — HardwareModel `BETAFPV_SUPERP_2400_RX` (136)
>
> **Status: the SX1280 LoRa radio is NOT usable under Meshtastic on this
> board.** The ESP32, BLE, WiFi and the RGB LED all work. Pins/power/wiring
> have been ruled out (ExpressLRS runs the radio perfectly on the same unit).
> The blocker is the generic RadioLib SX128x driver vs this board's SPI/timing
> characteristics — see the full investigation below.

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
| **LoRa SX1280**      | **NOT WORKING under Meshtastic/RadioLib**                    |

Meshtastic records `CriticalErrorCode_NO_RADIO` (3) and the device cannot mesh.

## What works in this variant

These changes are solid and stand on their own; they are unrelated to the radio
problem and are intentionally narrow:

- **Root `platformio.ini`**: `default_envs` switched to the newline-list form
  (space-separated is parsed by PlatformIO as a single env name and fails).
- **Status LED**: `ENABLE_AMBIENTLIGHTING` for this target plus a generic,
  macro-guarded fixed-colour override in `src/AmbientLightingThread.h`
  (`AMBIENTLIGHTING_RED/GREEN/BLUE/CURRENT`) — **no effect on other boards**
  that use `ENABLE_AMBIENTLIGHTING` (they don't define the macros). The SuperP
  shows a steady green "alive" LED.
- **Second radio parking**: `earlyInitVariant()` drives radio 2 NSS (GPIO27)
  high and its PA control (GPIO12) low and gives the primary chip a clean,
  well-delayed reset. Correct hygiene for a true-diversity board regardless of
  whether the radio works.
- **`LORA_SPI_FREQUENCY`** macro added to `src/mesh/RadioInterface.cpp`
  (default 4 MHz, unchanged for every other board) so the LoRa SPI clock is
  per-target tunable without touching common code.

## The radio investigation — what we found, in order

The failure mode is best read as a sequence: every fix uncovered the next
layer, and at the end the picture became clear.

### 1. First diagnostic: chip is reachable, the version-read corrupts

With `RADIOLIB_DEBUG_SPI` enabled the chip clearly responds to short commands
(status byte `0x43`/`0x45`/`0x55`/`0x65` — all "STDBY + cmd OK") but the
16-byte `RADIOLIB_SX128X_REG_VERSION_STRING` read is byte-shifted by a
varying amount and trails into `0xFF`:

- Cold first boot once read the full string `SX1280 V3B A9B7`.
- Subsequent boots read `SX1280 V3` then degrade to `0xFF` at a random offset,
  e.g. `SX1280)......`, `... 33 42 20 41 39 42 37 03` (tail "3B A9B7" shifted).

→ The chip drops MISO mid-burst. The 16-byte version read sometimes completes,
sometimes truncates. This is **not** between-command BUSY (it's a single SPI
transaction).

### 2. Things that did NOT fix it

| Hypothesis                          | What happened |
|-------------------------------------|---------------|
| Wrong pins                          | 100% match to official ELRS layout; ELRS works |
| Power / brownout                    | Reproduced on a 2S LiPo with no brownout; still `-2` |
| Second SX1280 contending the bus    | NSS2/GPIO27 confirmed HIGH; failures are `0xFF` (idle), not `0x00` (contention) |
| SPI clock too fast                  | Lowered to 1 MHz via `LORA_SPI_FREQUENCY` — identical garbage |
| Letting variant own the reset (`SX128X_RESET = RADIOLIB_NC`) | Made it strictly worse: chip then returned all `0xFF` (RadioLib's reset is required) |
| BUSY pin (GPIO37) not wired/readable | Disproved: post-reset probe measured `everHigh=1`, `highForUs=1395` — the pin works |

### 3. Patch attempts that produced progress

Sequence of incremental RadioLib patches applied via a `pre:` `extra_scripts`
hook (see git history of this branch for the script):

1. **Lengthen `SX128x::reset()`**: `delay(1)` → `delay(20)` low + 20 ms
   post-release settle. Result: the chip now finishes its internal boot before
   RadioLib starts SPI, so on a clean run the **version-string read is
   correct** (`SX1280 V3B A9B7`). Detection succeeds. The mid-burst truncation
   on warm reads is partially mitigated.
2. **Bump `Module::SPItransferStream` post-transfer pre-poll**:
   `delayMicroseconds(1)` → `delay(2)`/`delay(10)`. No effect on the next
   failure mode (see below); the chip dies right at `SetPacketType` (0x8A)
   regardless of how long we wait.
3. **Relax `SX128x::SPIparseStatus` 0x00/0xFF rejection**: this chip returns
   `SO FF FF` as the status during the `SetPacketType` write even when alive;
   removing the rejection lets RadioLib proceed. After this, the cold/UNSET
   path got far enough to return `-12` (`INVALID_FREQUENCY`, expected at 906
   MHz), Meshtastic auto-set `LORA_24` and rebooted. On the warm reboot init
   then failed with `-20` = `RADIOLIB_ERR_WRONG_MODEM`: a *read* of
   `GET_PACKET_TYPE` (CMDR 0x03) returned `0xFF` and RadioLib concluded "not
   LoRa".
4. **Short-circuit `SX128x::getPacketType()`** to always return
   `PACKET_TYPE_LORA` (we know we set LoRa in `config()`). Now `lora.begin()`
   itself returned `RADIOLIB_ERR_NONE` and Meshtastic logged
   `Frequency set to 2420.718750`, `Bandwidth set to 812.500000`,
   `Power output set to 3`. But the very next call (`setCRC(2)` /
   `startReceive()`, which issues `SetPacketParams` 0x8C) hit an invalid
   status `0x4F` (`STATUS_CMD_TIMEOUT`) → init returned non-zero → still
   `No SX1280 radio`.

### 4. Conclusion

The pattern across every patch is consistent:

- **SPI writes** are accepted by the chip (it responds with a sensible status
  byte for most commands).
- **SPI reads** (and the status byte on some writes) return **`0xFF`
  intermittently** — the chip stops driving MISO during reads at unpredictable
  byte offsets.

ExpressLRS works on the same hardware because its hand-written SX1280 driver
either does not perform these reads or tolerates the garbage. Every defensive
check in RadioLib that we peel away exposes the next read-based check.
Continuing down this path produces a firmware that lies to itself about chip
state; even if `begin()` were forced to return success, the runtime operation
of LoRa (reading `IrqStatus`, `RxBufferStatus`, `PacketStatus`, payload) would
remain unreliable because those code paths also depend on SPI reads.

This is a **RadioLib SX128x ↔ this specific diversity board** integration
problem at the SPI read level. It cannot be fixed at the variant configuration
layer.

## Options to pursue this further (if anyone is so inclined)

1. **Upstream / fork RadioLib's SX128x driver** to (a) be robust against
   spurious `0xFF` on writes, (b) verify reads via the BUSY edge / repeat-on-
   garbage, and (c) optionally provide an "ELRS-style trust-and-go" mode for
   diversity boards. This needs hardware-in-the-loop iteration — it is not
   something a remote patch session can converge on.
2. **Investigate the SuperP electrical layout**: probe the shared MISO trace
   with a scope while issuing back-to-back read commands, and confirm whether
   the second SX1280 (or the AT2401C front-ends, or the diversity routing) is
   the source of the marginal read timing. ELRS's working driver hints that
   the right SPI/BUSY sequence side-steps it.
3. **Accept as a known limitation** (current state of this branch): ship the
   target with BLE/WiFi/LED working and the radio documented as not yet
   functional under Meshtastic.

## How to reproduce / collect logs

```bash
git checkout claude/fix-betafpv-target-tlBdb
pio run -e betafpv_superp_2400 -t upload && pio run -e betafpv_superp_2400 -t monitor
```

Serial is UART0 (GPIO1 TX / GPIO3 RX) at 115200 — solder a 3.3 V USB-UART to
the pads (the RX has no USB).

To re-enable the diagnostics that produced this investigation, add to this
target's `platformio.ini` `build_flags`:

```
-DRADIOLIB_DEBUG_BASIC=1
-DRADIOLIB_DEBUG_SPI=1
-DRADIOLIB_DEBUG_PORT=Serial
```

The full set of incremental RadioLib patches that produced each step of
progress lives in this branch's git history under
`variants/esp32/betafpv_superp_2400/radiolib_busy_patch.py` (now removed from
the working tree).
