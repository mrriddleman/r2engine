import os
import platform

def GenerateBinaryData(flatcPath, outputDir, flatbufferPath, fbsName, sourcePath):
	
	osCall = flatcPath + " -b -o " + outputDir + " " + flatbufferPath + fbsName + " " + sourcePath
	os.system(osCall)
	return

platformSystem = platform.system()

flatc = "flatc"

if platformSystem == "Windows":
	flatc = "Windows/" + flatc + ".exe"
elif platformSystem == "Darwin":
	flatc = "MacOSX/" + flatc

thisFilePath = os.path.dirname(os.path.abspath(__file__))
flatcPath = os.path.realpath(thisFilePath + "/..") + "/vendor/flatbuffers/bin/" + flatc
srcPath = os.path.realpath(thisFilePath + "/..") + "/assets/models/raw/"
binPath = os.path.realpath(thisFilePath + "/..") + "/assets/models/bin/"
schemaPath = os.path.realpath(thisFilePath + "/..") + "/data/flatbuffer_schemas/"

for filename in os.listdir(srcPath):
	filepath = srcPath + filename
	if os.path.exists(filepath):
		GenerateBinaryData(flatcPath, binPath, schemaPath, "Model.fbs", filepath)