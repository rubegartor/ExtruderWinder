#!/usr/bin/env python3

Import("env")
import os
import re
import shutil

def apply_h7_video_fix(*args, **kwargs):
    """
    Aplica el fix de la PR https://github.com/arduino/ArduinoCore-mbed/pull/1052
    para solucionar el problema con el buffer flush size en Arduino_H7_Video
    """
    
    # Buscar el archivo Arduino_H7_Video.cpp en las librerías
    h7_video_files = []
    
    # Buscar en libdeps del proyecto
    lib_deps_dir = env.subst("$PROJECT_LIBDEPS_DIR")
    env_name = env.subst("$PIOENV")
    
    env_lib_dir = os.path.join(lib_deps_dir, env_name)
    if os.path.exists(env_lib_dir):
        for root, dirs, files in os.walk(env_lib_dir):
            for file in files:
                if file == "Arduino_H7_Video.cpp":
                    h7_video_files.append(os.path.join(root, file))
    
    # Buscar en el framework de PlatformIO
    try:
        import platform
        import pathlib
        
        if platform.system() == "Windows":
            framework_path = pathlib.Path("C:/.platformio/packages/framework-arduino-mbed/libraries/Arduino_H7_Video/src/Arduino_H7_Video.cpp")
        else:
            framework_path = pathlib.Path.home() / ".platformio/packages/framework-arduino-mbed/libraries/Arduino_H7_Video/src/Arduino_H7_Video.cpp"
        
        if framework_path.exists():
            h7_video_files.append(str(framework_path))
    except:
        pass
    
    if not h7_video_files:
        # print(f"Advertencia: No se encontró Arduino_H7_Video.cpp en {env_name}")
        return
    
    for h7_video_file in h7_video_files:
        process_file(h7_video_file, env.subst("$PIOENV"))

def process_file(h7_video_file, env_name):
    """Procesa un archivo específico"""
    # Leer el contenido del archivo
    try:
        with open(h7_video_file, 'r', encoding='utf-8') as f:
            content = f.read()
    except:
        return
    
    # Verificar si el patch ya está aplicado
    if "w * h * 2" in content and "free(rotated_buf)" in content:
        print(f"✓ Fix de Arduino_H7_Video ya aplicado en {env_name}")
        return
    
    print(f"→ Aplicando fix de Arduino_H7_Video en {env_name}: {h7_video_file}")
    
    # Crear backup
    backup_file = h7_video_file + ".original"
    if not os.path.exists(backup_file):
        shutil.copy2(h7_video_file, backup_file)
    
    # Aplicar el fix 1: Cambiar el tamaño del buffer de w * h * 4 a w * h * 2
    # y agregar verificación de memoria nula
    pattern1 = r'rotated_buf = \(uint8_t\*\)realloc\(rotated_buf, w \* h \* 4\);'
    replacement1 = '''uint8_t* new_buf = (uint8_t*)realloc(rotated_buf, w * h * 2);
    if (new_buf == NULL) {
      return; // Insufficient memory error
    }
    rotated_buf = new_buf;'''
    
    content = re.sub(pattern1, replacement1, content)
    
    # Aplicar el fix 2: Agregar liberación del buffer después de dsi_lcdDrawImage
    # Buscar la línea específica y agregar el código después
    pattern2 = r'(dsi_lcdDrawImage\([^)]+\)[^;]*;)'
    replacement2 = r'''\1
  
  // free the rotated buffer if it was allocated
  if (rotation != LV_DISPLAY_ROTATION_0) {
    free(rotated_buf);
    rotated_buf = nullptr;
  }'''
    
    content = re.sub(pattern2, replacement2, content, flags=re.MULTILINE)
    
    # Escribir el archivo modificado
    try:
        with open(h7_video_file, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"✓ Fix de Arduino_H7_Video aplicado exitosamente en {env_name}")
    except:
        print(f"✗ Error escribiendo archivo {h7_video_file}")

# Ejecutar el fix inmediatamente si ya existen las librerías
apply_h7_video_fix()

# También registrar para después de la instalación de librerías
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", apply_h7_video_fix)
