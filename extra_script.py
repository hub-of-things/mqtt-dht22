## https://github.com/mcuw/ESP32-ghbuild-template/blob/main/extra_script.py
## firmware rename
import os

Import("env")

# Access to global construction environment
build_tag = env['PIOENV']
version_tag = os.getenv("FIRMWARE_VERSION")

env.Replace(PROGNAME="firmware_%s_%s" % (build_tag, version_tag))

# esto es para la el scan con sonarqube
# include toolchain paths
env.Replace(COMPILATIONDB_INCLUDE_TOOLCHAIN=True)

# override compilation DB path
env.Replace(COMPILATIONDB_PATH=os.path.join("$BUILD_WRAPPER_OUT_DIR", "compile_commands.json"))