
include "../../vendor/lz4"

libIncludeDirs = {}
libIncludeDirs["lz4"] = "../../vendor/lz4/include"
libIncludeDirs["flatbuffers"] = "../../vendor/flatbuffers/include"
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
		"src/*.cpp"
	}

	includedirs
	{
		"include",
		"%{libIncludeDirs.lz4}",
		"%{libIncludeDirs.flatbuffers}"
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