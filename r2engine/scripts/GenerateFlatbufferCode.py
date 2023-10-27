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
flatcPath = os.path.realpath(thisFilePath + "/..") + "/vendor/flatbuffers/bin/" + flatc
dataPath = os.path.realpath(thisFilePath + "/..") + "/data/flatbuffer_schemas/"
fbsCodeOutputMap = {
"AssetManifest.fbs": "/r2/Core/Assets/Pipeline/",
"SoundDefinition.fbs": "/r2/Audio/",
"ShaderManifest.fbs": "/r2/Core/Assets/Pipeline/",
"Utils.fbs": "/r2/Utils/",
"Mesh.fbs" : "/r2/Render/Model/",
"Model.fbs": "/r2/Render/Model/",
"TexturePackManifest.fbs": "/r2/Render/Model/Textures/",
"TexturePackMetaData.fbs": "/r2/Render/Model/Textures/",
"TextureMetaData.fbs": "/../libs/assetlib/include/assetlib/",
"RModelMetaData.fbs": "/../libs/assetlib/include/assetlib/",
"RModel.fbs":"/../libs/assetlib/include/assetlib/",
"RAnimationMetaData.fbs":"/../libs/assetlib/include/assetlib/",
"RAnimation.fbs":"/../libs/assetlib/include/assetlib/",
"EntityData.fbs":"/r2/Game/ECS/",
"ComponentArrayData.fbs":"/r2/Game/ECS/",
"LevelData.fbs":"/r2/Game/Level/",
"LevelPack.fbs":"/r2/Game/Level/",
"ShaderEffect.fbs":"/r2/Render/Model/Shader/",
"ShaderEffectPasses.fbs":"/r2/Render/Model/Shader/",
"ShaderParams.fbs":"/r2/Render/Model/Shader/",
"Material.fbs":"/r2/Render/Model/Materials/",
"MaterialPack.fbs":"/r2/Render/Model/Materials/",
"ComponentArraySchemas/EditorComponentArrayData.fbs":"/r2/Game/ECS/Serialization/",
"ComponentArraySchemas/HeirarchyComponentArrayData.fbs":"/r2/Game/ECS/Serialization/",
"ComponentArraySchemas/InstancedSkeletalAnimationComponentArrayData.fbs":"/r2/Game/ECS/Serialization/",
"ComponentArraySchemas/InstancedTransformComponentArrayData.fbs":"/r2/Game/ECS/Serialization/",
"ComponentArraySchemas/RenderComponentArrayData.fbs":"/r2/Game/ECS/Serialization/",
"ComponentArraySchemas/SkeletalAnimationComponentArrayData.fbs":"/r2/Game/ECS/Serialization/",
"ComponentArraySchemas/TransformComponentArrayData.fbs":"/r2/Game/ECS/Serialization/",
"ComponentArraySchemas/AudioListenerComponentArrayData.fbs": "/r2/Game/ECS/Serialization/",
"ComponentArraySchemas/AudioEmitterComponentArrayData.fbs":"/r2/Game/ECS/Serialization/",
}

extraParams = {
"AssetManifest.fbs": "",
"SoundDefinition.fbs": "",
"ShaderManifest.fbs": "",
"Utils.fbs": "",
"Mesh.fbs" : "",
"Model.fbs": "",
"TexturePackManifest.fbs": "",
"TexturePackMetaData.fbs": "",
"TextureMetaData.fbs": "--gen-mutable",
"RModelMetaData.fbs":"--gen-mutable",
"RModel.fbs":"--gen-mutable",
"RAnimationMetaData.fbs":"--gen-mutable",
"RAnimation.fbs":"--gen-mutable",
"EntityData.fbs":"",
"ComponentArrayData.fbs":"",
"LevelData.fbs": "",
"LevelPack.fbs": "",
"ShaderEffect.fbs":"",
"ShaderEffectPasses.fbs":"",
"ShaderParams.fbs":"",
"Material.fbs":"",
"MaterialPack.fbs":"",
"ComponentArraySchemas/EditorComponentArrayData.fbs":"",
"ComponentArraySchemas/HeirarchyComponentArrayData.fbs":"",
"ComponentArraySchemas/InstancedSkeletalAnimationComponentArrayData.fbs":"",
"ComponentArraySchemas/InstancedTransformComponentArrayData.fbs":"",
"ComponentArraySchemas/RenderComponentArrayData.fbs":"",
"ComponentArraySchemas/SkeletalAnimationComponentArrayData.fbs":"",
"ComponentArraySchemas/TransformComponentArrayData.fbs":"",
"ComponentArraySchemas/AudioListenerComponentArrayData.fbs":"",
"ComponentArraySchemas/AudioEmitterComponentArrayData.fbs":"",
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