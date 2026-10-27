"""Compile/run the same portable parser and renderer used by the ESP32 app.

Requires a C++17 compiler, and libraries installed by `pio pkg install`.
Use --compiler 'zig c++' when GCC/Clang is unavailable.
"""
from pathlib import Path
import argparse, subprocess, shlex, os
p=argparse.ArgumentParser()
p.add_argument('--compiler',default='g++')
a=p.parse_args()
root=Path(__file__).resolve().parents[1]
os.chdir(root)
build=root/'tests/build'
build.mkdir(parents=True,exist_ok=True)
libs=root/'.pio/libdeps/crowpanel_green'
json_include=libs/'ArduinoJson/src'
gfx=libs/'Adafruit GFX Library'
if not json_include.exists():raise SystemExit('Run pio pkg install -e crowpanel_green first')
compiler=shlex.split(a.compiler,posix=os.name!='nt')
suffix='.exe' if os.name=='nt' else ''
base=compiler+['-std=c++17','-O0','-Iinclude','-I'+str(json_include)]
exe=build/('core-tests'+suffix)
subprocess.run(base+['tests/core_tests.cpp','src/core.cpp','-o',str(exe)],check=True)
subprocess.run([str(exe)]+([] if os.name=='nt' else ['--dst']),check=True)
subprocess.run([str(exe),'--live-fixture','tests/fixtures/open-meteo.json'],check=True)
render=build/('render-test'+suffix)
flags=['-DARDUINO=10800','-DARDUINOJSON_ENABLE_ARDUINO_STRING=0','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0','-DARDUINOJSON_ENABLE_PROGMEM=0','-Itests/stubs','-I.','-I'+str(gfx)]
subprocess.run(base+flags+['tests/render_test.cpp','src/core.cpp','src/screen.cpp',str(gfx/'Adafruit_GFX.cpp'),'-o',str(render)],check=True)
subprocess.run([str(render),'tests/fixtures/open-meteo.json'],check=True)
try:
    from PIL import Image
    for file in build.glob('*.pbm'):Image.open(file).save(file.with_suffix('.png'))
except ImportError: pass
