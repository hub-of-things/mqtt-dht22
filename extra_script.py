## https://github.com/mcuw/ESP32-ghbuild-template/blob/main/extra_script.py
## firmware rename
import os

Import("env")

# Access to global construction environment
build_tag = env['PIOENV']
version_tag = os.getenv("FIRMWARE_VERSION")

env.Replace(PROGNAME="firmware_%s_%s" % (build_tag, version_tag))