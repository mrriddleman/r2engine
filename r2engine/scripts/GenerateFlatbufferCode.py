import os
import platform

def GeneratedCode(flatcPath, codeOutputDir, flatbufferPath, fbsName):
	osCall = flatcPath + " -c -o " + codeOutputDir + " " + flatbufferPath + fbsName
	os.system(osCall)
	return

platformSystem = platform.system()

flatc = "flatc"

if platformSystem == "Windows":
	flatc = "Windows/" + flatc + ".exe"
elif platformSystem == "Darwin":
	flatc = "MacOSX/" + flatc

thisFilePath = os.path.dirname(os.path.abspath(__file__))
srcPath = os.path.realpath(thisFilePath + "/../src")
flatcPath = os.path.realpath(thisFilePath + "/..") + "/vendor/flatbuffers/bin/" + flatc
dataPath = os.path.realpath(thisFilePath + "/..") + "/data/flatbuffer_schemas/"

fbsCodeOutputMap = {"AssetManifest.fbs": "/r2/Core/Assets/Pipeline/", "SoundDefinition.fbs": "/r2/Audio/"}

for filename in os.listdir(dataPath):
	outputPath = srcPath + fbsCodeOutputMap[filename]
	if os.path.exists(outputPath):
		GeneratedCode(flatcPath, outputPath, dataPath, filename)

