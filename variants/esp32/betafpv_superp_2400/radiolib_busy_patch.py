Import("env")
import glob
import os

# Two RadioLib timing weaknesses on the BetaFPV SuperP SX1280:
#
# 1) SX128x::reset() pulses NRESET low for only delay(1) and then immediately
#    starts SPI (standby/version read) WITHOUT waiting for the chip to finish
#    its internal boot. ExpressLRS resets generously and waits. The version
#    read then catches the chip mid-boot -> reads truncate to 0xFF at a varying
#    byte -> intermittent "found but garbage" -> init -2.
#
# 2) Module::SPItransferStream() only delays 1 us after a transfer before
#    sampling BUSY, which is shorter than this SX1280's BUSY-assert latency.
#
# This pre-build hook patches both in the downloaded RadioLib. Idempotent
# (marker RADIOLIB_SUPERP_BUSY_PATCH). Only wired into the betafpv_superp_2400
# env. NOTE: on a pristine .pio the libs may not be downloaded the first time
# this runs; the build log will say "not found yet" -> just build again.

MARKER = "RADIOLIB_SUPERP_BUSY_PATCH"

PATCHES = {
    os.path.join("src", "Module.cpp"): [
        ("      this->hal->delayMicroseconds(1);",
         "      this->hal->delay(2); // " + MARKER + " (bring-up value; tune down once stable)"),
    ],
    os.path.join("src", "modules", "SX128x", "SX128x.cpp"): [
        # longer NRESET low + a post-release settle so the chip finishes its
        # internal boot before RadioLib reads the version register
        ("  this->mod->hal->digitalWrite(this->mod->getRst(), this->mod->hal->GpioLevelLow);\n"
         "  this->mod->hal->delay(1);\n"
         "  this->mod->hal->digitalWrite(this->mod->getRst(), this->mod->hal->GpioLevelHigh);",
         "  this->mod->hal->digitalWrite(this->mod->getRst(), this->mod->hal->GpioLevelLow);\n"
         "  this->mod->hal->delay(20); // " + MARKER + "\n"
         "  this->mod->hal->digitalWrite(this->mod->getRst(), this->mod->hal->GpioLevelHigh);\n"
         "  this->mod->hal->delay(20); // " + MARKER + " post-reset settle"),
        # SPIparseStatus rejects 0x00/0xFF status as CHIP_NOT_FOUND. On this
        # board the chip occasionally returns 0xFF as the status byte during a
        # write (SetPacketType) while still being alive, and RadioLib bails out
        # of begin() with -2. ExpressLRS does not check write status at all.
        # Drop the 0x00/0xFF rejection here; findChip()'s strncmp("SX1280",...)
        # already handles real chip absence.
        ("  } else if((in == 0x00) || (in == 0xFF)) {\n"
         "    return(RADIOLIB_ERR_CHIP_NOT_FOUND);\n"
         "  }",
         "  } // " + MARKER + ": drop 0x00/0xFF chip-absence reject; findChip handles it"),
        # SPI *reads* on this board also return 0xFF intermittently (e.g.,
        # GetPacketType after SetRfFrequency), making RadioLib bail with
        # WRONG_MODEM (-20) even though config() just set LoRa. We always run
        # this target in LoRa mode (Meshtastic), so short-circuit getPacketType
        # to return the modem we explicitly configured.
        ("uint8_t SX128x::getPacketType() {\n"
         "  uint8_t data = 0xFF;\n"
         "  this->mod->SPIreadStream(RADIOLIB_SX128X_CMD_GET_PACKET_TYPE, &data, 1);\n"
         "  return(data);\n"
         "}",
         "uint8_t SX128x::getPacketType() {\n"
         "  // " + MARKER + ": SPI reads return 0xFF intermittently on this board;\n"
         "  // trust the modem we set in config() (Meshtastic always uses LoRa).\n"
         "  return(RADIOLIB_SX128X_PACKET_TYPE_LORA);\n"
         "}"),
    ],
}


def find_radiolib_root():
    libdeps = env.subst("$PROJECT_LIBDEPS_DIR")
    pioenv = env["PIOENV"]
    for base in (os.path.join(libdeps, pioenv), libdeps):
        hits = glob.glob(os.path.join(base, "**", "RadioLib", "src", "Module.cpp"),
                         recursive=True)
        if hits:
            return os.path.dirname(os.path.dirname(hits[0]))  # .../RadioLib
    return None


root = find_radiolib_root()
if not root:
    print("[superp busy patch] RadioLib not downloaded yet; SKIPPING - "
          "run the build once more so the patch can apply")
else:
    for rel, repls in PATCHES.items():
        path = os.path.join(root, rel)
        if not os.path.isfile(path):
            print("[superp busy patch] WARNING: %s missing" % path)
            continue
        with open(path, "r") as f:
            src = f.read()
        if MARKER in src:
            print("[superp busy patch] already applied: %s" % path)
            continue
        changed = False
        for old, new in repls:
            if old in src:
                src = src.replace(old, new, 1)
                changed = True
            else:
                print("[superp busy patch] WARNING: pattern not found in %s "
                      "(RadioLib changed?)" % path)
        if changed:
            with open(path, "w") as f:
                f.write(src)
            print("[superp busy patch] APPLIED: %s" % path)
