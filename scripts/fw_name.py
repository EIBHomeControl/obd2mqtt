# PlatformIO extra_script: kopiert nach dem Build die Firmware mit Versionsnummer nach firmware/
#   firmware/obd2mqtt-v<VERSION>-<env>-ota.bin
import os, re, shutil
Import("env")

def version():
    for f in env.get("BUILD_FLAGS", []):
        m = re.search(r'FW_VERSION=\\?"([^"\\]+)', str(f))
        if m:
            return m.group(1)
    return "dev"

def copy_fw(source, target, env):
    out = os.path.join(env.subst("$PROJECT_DIR"), "firmware")
    os.makedirs(out, exist_ok=True)
    dst = os.path.join(out, "obd2mqtt-v%s-%s-ota.bin" % (version(), env.subst("$PIOENV")))
    shutil.copyfile(str(target[0]), dst)
    print("Firmware kopiert nach", dst)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_fw)
