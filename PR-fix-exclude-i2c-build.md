# PR: Fix `MESHTASTIC_EXCLUDE_I2C` build + drop the RadioMaster Nomad I2C workaround

## Problem

Building a target with `MESHTASTIC_EXCLUDE_I2C` while the InputBroker is
compiled fails:

```
InputBroker.cpp: error: 'cardKbI2cImpl' was not declared in this scope
InputBroker.cpp: error: 'CardKbI2cImpl' was not declared in this scope
```

`InputBroker::Init()` (`src/input/InputBroker.cpp`) instantiates
`cardKbI2cImpl`, but the declaration — `#include "input/cardKbI2cImpl.h"` at
the top of the same file — is already gated behind `#if !MESHTASTIC_EXCLUDE_I2C`.
With I2C excluded, the type and the global are undeclared at the use site.
This is a genuine upstream bug: the include is guarded, the use is not.

The RadioMaster Nomad variant papered over this (and the telemetry-sensor
build) with a throwaway commit ("PEZZOTTO - workaround build", `fc400c7`):

- `-DHAS_TELEMETRY=0` in the variant `platformio.ini`
- the `cardKbI2cImpl` lines commented out
- edits to `src/Power.cpp`

`-DHAS_TELEMETRY=0` is a blunt instrument: it disables the whole Telemetry
module (device metrics — uptime, channel utilization, air-time), none of
which needs I2C.

## Fix

### Upstream bug fix

`src/input/InputBroker.cpp` — guard the `cardKbI2cImpl` instantiation with
`#if !MESHTASTIC_EXCLUDE_I2C`, matching the guard already used for its
`#include`. No-op when I2C is enabled; this can stand alone as an upstream PR.

### RadioMaster Nomad variant cleanup

- `variants/esp32/radiomaster_nomad/platformio.ini`: remove `-DHAS_TELEMETRY=0`.
  Instead exclude only the I2C-bound sensor sub-systems, exactly as the
  shipped `heltec_wireless_bridge` variant already does for an
  `MESHTASTIC_EXCLUDE_I2C` ESP32 build:
  - `MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR`
  - `MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR_EXTERNAL`
  - `MESHTASTIC_EXCLUDE_AIR_QUALITY_SENSOR`

  Device telemetry stays enabled.

- `src/Power.cpp`: reverted to upstream. The workaround's edits were
  redundant — those blocks are `HAS_TELEMETRY`-gated and now compile out via
  the exclude flags — and one of them introduced a latent type mismatch
  (`extern MAX17048Sensor max17048Sensor` in `power.h` vs a `NullSensor`
  definition).

- `src/power.h`: keeps the earlier fix (commit `3b2da19`) — the
  `extern MAX17048Sensor` declaration is gated on `!MESHTASTIC_EXCLUDE_I2C`
  to match `MAX17048Sensor.h` and `Power.cpp`.

## Out of scope

The workaround commit also changed `.vscode/extensions.json` and
`userPrefs.jsonc` (timezone string). Those are unrelated to the I2C build and
are left untouched.

## Verification status

**Not compile-verified.** The build environment used here cannot download the
PlatformIO ESP32 platform (network allowlist — `HTTPClientError: Host not in
allowlist`). The fix is reasoned from the source and mirrors the known-good
`heltec_wireless_bridge` configuration. Run a real build before merging.

## Test plan

- [ ] `pio run -e radiomaster_nomad` builds clean
- [ ] `pio run -e heltec-wireless-bridge` still builds (InputBroker change is global)
- [ ] A board with the CardKB I2C keyboard still enumerates it (the InputBroker
      change is a no-op when I2C is enabled)
- [ ] Nomad emits device-telemetry packets (regression from `HAS_TELEMETRY=0`)
