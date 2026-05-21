#!/usr/bin/env python3
# trunk-ignore-all(ruff/F821)
# trunk-ignore-all(flake8/F821): For SConstruct imports
Import("env")
platform = env.PioPlatform()

if platform.name == "native":
    env.Replace(PROGNAME="meshtasticd")
else:
    from readprops import readProps
    prefsLoc = env["PROJECT_DIR"] + "/version.properties"
    verObj = readProps(prefsLoc)
    env.Replace(PROGNAME=f"firmware-{env.get('PIOENV')}-{verObj['long']}")
    env.Replace(ESP32_FS_IMAGE_NAME=f"littlefs-{env.get('PIOENV')}-{verObj['long']}")

# Print the new program name for verification
print(f"PROGNAME: {env.get('PROGNAME')}")
if platform.name == "espressif32":
    print(f"ESP32_FS_IMAGE_NAME: {env.get('ESP32_FS_IMAGE_NAME')}")


# RadioLib LR1121 firmware-id compatibility patch.
# An LR1121 running its transceiver firmware reports GetVersion device id 0xF3
# (see ExpressLRS LR1121Driver: LR1121_FIRMWARE_TYPE). Upstream RadioLib's
# LR11x0::findChip only accepts the silicon/bootloader id 0x03, so it fails to
# detect an otherwise healthy chip (e.g. on the RadioMaster Nomad). Teach
# findChip to also accept 0xF3 for the LR1121. The edit is additive (0x03 still
# works), idempotent, and harmless to every other board.
def patch_radiolib_lr1121(*_args, **_kwargs):
    import os

    libdeps = env.get("PROJECT_LIBDEPS_DIR")
    pioenv = env.get("PIOENV")
    if not libdeps or not pioenv:
        return
    src = os.path.join(libdeps, pioenv, "RadioLib", "src", "modules", "LR11x0", "LR11x0.cpp")
    if not os.path.isfile(src):
        return
    with open(src, "r", encoding="utf-8", errors="ignore") as fh:
        text = fh.read()
    marker = "/* meshtastic: also accept LR1121 fw id 0xF3 */"
    if marker in text:
        return
    # findChip's local variable was renamed info -> versionInfo in newer
    # RadioLib; accept either spelling.
    needle = None
    for var in ("versionInfo", "info"):
        candidate = "(%s.device == RADIOLIB_LR11X0_DEVICE_BOOT)" % var
        if candidate in text:
            needle, devvar = candidate, var
            break
    if needle is None:
        print("WARNING: RadioLib LR11x0 0xF3 patch anchor not found -- "
              "LR1121 transceiver firmware will not be detected!")
        return
    text = text.replace(
        needle,
        needle + " || ((ver == RADIOLIB_LR11X0_DEVICE_LR1121) && (%s.device == 0xF3)) " % devvar + marker,
        1,
    )
    with open(src, "w", encoding="utf-8") as fh:
        fh.write(text)
    print("Patched RadioLib LR11x0::findChip to accept LR1121 firmware id 0xF3")


patch_radiolib_lr1121()
env.AddPreAction("$BUILD_DIR/RadioLib/src/modules/LR11x0/LR11x0.cpp.o", patch_radiolib_lr1121)
