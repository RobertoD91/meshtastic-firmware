Import("env")
import glob
import os

# RadioLib's Module::SPItransferStream() waits for the BUSY GPIO after each SPI
# transfer, but only delays 1 us before sampling it. On the BetaFPV SuperP the
# SX1280 ("V3B A9B7") asserts BUSY later than 1 us, so RadioLib sees BUSY still
# low, assumes the command finished, and fires the next command into a chip
# that is about to go busy -> corrupted reads / chip drops off -> init -2.
# ExpressLRS works because it waits on BUSY generously. This pre-build hook
# bumps that single pre-sample delay so BUSY has time to rise. Idempotent; only
# wired into the betafpv_superp_2400 env.

MARKER = "RADIOLIB_SUPERP_BUSY_PATCH"
OLD = "      this->hal->delayMicroseconds(1);"
NEW = ("      this->hal->delayMicroseconds(50); // " + MARKER +
       ": let SX1280 assert BUSY before sampling")

libdeps = env.subst("$PROJECT_LIBDEPS_DIR")
pioenv = env["PIOENV"]
matches = glob.glob(os.path.join(libdeps, pioenv, "**", "RadioLib", "src", "Module.cpp"),
                    recursive=True)
if not matches:
    matches = glob.glob(os.path.join(libdeps, "**", "RadioLib", "src", "Module.cpp"),
                        recursive=True)

if not matches:
    print("[superp busy patch] RadioLib Module.cpp not found yet; skipping")
else:
    path = matches[0]
    with open(path, "r") as f:
        src = f.read()
    if MARKER in src:
        print("[superp busy patch] already applied: %s" % path)
    elif OLD in src:
        with open(path, "w") as f:
            f.write(src.replace(OLD, NEW, 1))
        print("[superp busy patch] applied to %s" % path)
    else:
        print("[superp busy patch] WARNING: expected line not found in %s "
              "(RadioLib changed?) - not patched" % path)
