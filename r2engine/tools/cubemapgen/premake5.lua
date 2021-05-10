
cubemapgenIncludeDirs = {}
cubemapgenIncludeDirs["ibl"] = "../../libs/ibl/include"
cubemapgenIncludeDirs["glm"] = "../../vendor/glm"
cubemapgenIncludeDirs["cmdln"] = "../../libs/cmdln/include"
cubemapgenIncludeDirs["stb"] = "../../vendor/stb"
cubemapgenIncludeDirs["flatbuffers"] = "../../vendor/flatbuffers/include"

project "cubemapgen"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"include/*.h",
		"src/*.cpp",
		
	}

	includedirs
	{
		"include",
		"%{cubemapgenIncludeDirs.ibl}",
		"%{cubemapgenIncludeDirs.glm}",
		"%{cubemapgenIncludeDirs.cmdln}",
		"%{cubemapgenIncludeDirs.stb}",
		"%{cubemapgenIncludeDirs.flatbuffers}",
		"../../src/r2/Render/Model/Textures"
		
	}

	configurations
	{
		"Debug",
		"Release",
		"Publish"
	}

	links
	{
		"ibl",
		"cmdln"
	}

	filter "system:windows"
		flags {"MultiProcessorCompile"}
		defines{
			'R2_FLATC="'..os.getcwd()..'/../../vendor/flatbuffers/bin/Windows/flatc.exe"',
			'R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH="'..os.getcwd()..'/../../data/flatbuffer_schemas"'
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