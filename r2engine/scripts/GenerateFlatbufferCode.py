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
fbsCodeOutputMap = {
"AssetManifest.fbs": "/r2/Core/Assets/Pipeline/",
"SoundDefinition.fbs": "/r2/Audio/",
"ShaderManifest.fbs": "/r2/Core/Assets/Pipeline/",
"Utils.fbs": "/r2/Utils/",
"Material.fbs": "/r2/Render/Model/",
"MaterialPack.fbs": "/r2/Render/Model/",
"Mesh.fbs" : "/r2/Render/Model/",
"Model.fbs": "/r2/Render/Model/",
"TexturePackManifest.fbs": "/r2/Render/Model/Textures/",
"TexturePackMetaData.fbs": "/r2/Render/Model/Textures/",
"MaterialParams.fbs":"/r2/Render/Model/",
"MaterialParamsPack.fbs":"/r2/Render/Model/",
"TextureMetaData.fbs": "/../libs/assetlib/include/assetlib/"
}

for filename in os.listdir(dataPath):
	if(filename in fbsCodeOutputMap):
		outputPath = srcPath + fbsCodeOutputMap[filename]
		if os.path.exists(outputPath):
			GeneratedCode(flatcPath, outputPath, dataPath, filename)