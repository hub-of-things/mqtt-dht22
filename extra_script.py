## https://github.com/mcuw/ESP32-ghbuild-template/blob/main/extra_script.py
## firmware rename
import os
import shutil

Import("env")

# Access to global construction environment
build_tag = env['PIOENV']
version_tag = os.getenv("FIRMWARE_VERSION")

env.Replace(PROGNAME="firmware_%s_%s" % (build_tag, version_tag))

def after_build(target, source, env):
    source_file = ".pio/build/esp32thing/compile_commands.json"
    destination_dir = "build_wrapper_output_directory/"
    
    # Verifica si el archivo de origen existe
    if os.path.exists(source_file):
        # Verifica si el directorio de destino existe, si no lo crea
        if not os.path.exists(destination_dir):
            os.makedirs(destination_dir)
        
        # Copia el archivo
        shutil.copy(source_file, os.path.join(destination_dir, "compile_commands.json"))
    else:
        print(f"El archivo {source_file} no se encontró.")

# Esto se llama después de cada compilación
env.AddPostAction("buildprog", after_build)

