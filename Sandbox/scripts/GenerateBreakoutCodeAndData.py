import os
import platform


def GeneratedCode(fbsList, flatcPath, codeOutputDir, flatbufferPath):
	for nextFBS in fbsList:
		osCall = flatcPath + " -c -o " + codeOutputDir + " " + flatbufferPath + nextFBS
		os.system(osCall)
	return

def GenerateData(flatcPath, inputDir, outputDir):
	return


platformSystem = platform.system();

flatc = "flatc"

if platformSystem == "Windows":
	flatc = "Windows/" + flatc + ".exe"
elif platformSystem == "Darwin":
	flatc = "MacOSX/" + flatc

flatcPath = "r2engine/vendor/flatbuffers/bin/" + flatc
fbsList = ["BreakoutLevelSchema.fbs", "BreakoutHighScoreSchema.fbs", "BreakoutPlayerSettings.fbs", "BreakoutPowerupSchema.fbs"]
flatbufferPath = "engine_assets/Flatbuffer_Data/"
codeOutputDir = "./Sandbox/src/"

json = "engine_assets/Flatbuffer_Data/breakout_level_pack.json"
bin = "engine_assets/Flatbuffer_Data/breakout_level_pack.breakout_level"
outputDir = "engine_assets/Flatbuffer_Data/"

binDir = "engine_assets/Flatbuffer_Data/"

#need to figure out a way to map file to fbs file

fbsMap = {"breakout_high_scores": "BreakoutHighScoreSchema.fbs", "breakout_powerups": "BreakoutPowerupSchema.fbs", "breakout_level_pack": "BreakoutLevelSchema.fbs", "breakout_player_save": "BreakoutPlayerSettings.fbs"}


for filename in os.listdir(binDir):
	if filename.endswith(".bin"):
		filenameWithoutExt = os.path.splitext(filename)[0]
		jsonFileName = filenameWithoutExt + ".json"
		jsonOSCall = flatcPath + " -t -o " + outputDir + " " + outputDir + fbsMap[filenameWithoutExt] + " -- " + binDir + filename + " --raw-binary"
		#print(jsonOSCall)
		#print("fbsMap[" + filenameWithoutExt + "]: " + fbsMap[filenameWithoutExt])

		os.system(jsonOSCall)

		binOSCall = flatcPath + " -b -o " + outputDir + " " + outputDir + fbsMap[filenameWithoutExt] + " " + outputDir + jsonFileName

		os.system(binOSCall)








#jsonOSCall = flatcPath + " -b -o " + outputDir + " " + fbsPath + " " + json
#binOSCall = flatcPath + " -t -o " + outputDir + " " + fbsPath + " -- " + bin + " --raw-binary"


#os.system(jsonOSCall)
#os.system(binOSCall)