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
json = "engine_assets/Flatbuffer_Data/breakout_level_pack.json"
bin = "engine_assets/Flatbuffer_Data/breakout_level_pack.breakout_level"
outputDir = "engine_assets/Flatbuffer_Data/"

osCall = flatcPath + " -c -o ./Sandbox/src/ " + fbsPath
jsonOSCall = flatcPath + " -b -o " + outputDir + " " + fbsPath + " " + json
binOSCall = flatcPath + " -t -o " + outputDir + " " + fbsPath + " -- " + bin + " --raw-binary"

os.system(osCall)
os.system(jsonOSCall)
os.system(binOSCall)