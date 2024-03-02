import os
import platform

def GeneratedCode(flatcPath, codeOutputDir, flatbufferPath, fbsName, extraParam):
	osCall = flatcPath + " -c -o " + codeOutputDir + " " + flatbufferPath + fbsName + " " + extraParam
	print(osCall)
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
flatcPath = os.path.realpath(thisFilePath + "/../../r2engine") + "/vendor/flatbuffers/bin/" + flatc
dataPath = os.path.realpath(thisFilePath + "/..") + "/data/flatbuffer_schemas/"
fbsCodeOutputMap = {
"FacingComponentArrayData.fbs":"",
"GridPositionComponentArrayData.fbs":"",
"GridPosition.fbs":"",
"InstancedGridPositionComponentArrayData.fbs":""
}

extraParams = {
"FacingComponentArrayData.fbs": "",
"GridPositionComponentArrayData.fbs":"",
"GridPosition.fbs":"",
"InstancedGridPositionComponentArrayData.fbs":""
}

for filename in os.listdir(dataPath):
	if(filename in fbsCodeOutputMap):
		outputPath = srcPath + fbsCodeOutputMap[filename]
		extraParam = extraParams[filename]
		if os.path.exists(outputPath):
			GeneratedCode(flatcPath, outputPath, dataPath, filename, extraParam)
	if(os.path.isdir(dataPath + filename)):
		dirname = filename
		for subfilename in os.listdir(dataPath + dirname):
			fbsFileURI = (dirname +"/"+ subfilename)
			if(fbsFileURI in fbsCodeOutputMap):
				outputPath = srcPath + fbsCodeOutputMap[fbsFileURI]
				extraParam = extraParams[fbsFileURI]
				if os.path.exists(outputPath):
					GeneratedCode(flatcPath, outputPath, dataPath, fbsFileURI, extraParam)