import os
import platform

platformSystem = platform.system();

flatc = "flatc"

if platformSystem == "Windows":
	flatc = "Windows/" + flatc + ".exe"
elif platformSystem == "Darwin":
	flatc = "MacOSX/" + flatc


flatcPath = "r2engine/vendor/flatbuffers/bin/" + flatc
fbsPath = "engine_assets/Flatbuffer_Data/BreakoutLevelSchema.fbs"

osCall = flatcPath + " -c -o ./Sandbox/src/ " + fbsPath

os.system(osCall)