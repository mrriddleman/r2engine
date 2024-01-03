
include "../../vendor/lz4"

libIncludeDirs = {}
libIncludeDirs["lz4"] = "../../vendor/lz4/include"
libIncludeDirs["flatbuffers"] = "../../vendor/flatbuffers/include"
libIncludeDirs["stb"] = "../../vendor/stb"
libIncludeDirs["glm"] = "../../vendor/glm"
libIncludeDirs["fastgltf"] = "../../vendor/fastgltf"

flatcPath, err = os.realpath('.')

project "assetlib"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"include/assetlib/*.h",
		"src/*.cpp",
		"../../vendor/stb/*.cpp",
		"../../vendor/fastgltf/*.cpp"
	}

	includedirs
	{
		"include",
		"../../src/r2/Render/Model/Textures",
		"../../src/r2/Render/Model",
		"../../src/r2/Render/Model/Shader",
		"../../src/r2/Render/Model/Materials",
		"../../src/r2/Utils",
		"../../src/r2/Core/Assets/",
		"%{libIncludeDirs.lz4}",
		"%{libIncludeDirs.flatbuffers}",
		"%{libIncludeDirs.stb}",
		"%{libIncludeDirs.glm}",
		"%{libIncludeDirs.fastgltf}"
	}

	links
	{
		"lz4"
	}

	configurations
	{
		"Debug",
		"Release",
		"Publish"
	}

	filter "system:windows"
		flags {"MultiProcessorCompile"}
		defines
		{
			'FLATC="%{flatcPath}"'
		}

		sysincludedirs
		{
			"../../vendor/assimp/Windows/include"
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "On"
		optimize "Off"
		staticruntime "off"

	filter "configurations:Release"
		runtime "Release"
		symbols "Off"
		optimize "On"
		staticruntime "off"

	filter "configurations:Publish"
		runtime "Release"
		symbols "Off"
		optimize "On"
		staticruntime "off"