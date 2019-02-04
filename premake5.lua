workspace "r2"
	architecture "x64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release",
		"Publish"
	}

outputdir = "%{cfg.buildcfg}_%{cfg.system}_%{cfg.architecture}"

IncludeDir = {}

project "r2engine"
	location "r2engine"
	kind "SharedLib"
	language "C++"
	--staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin_int/" .. outputdir .. "/%{prj.name}")

	pchheader "src/r2pch.h"
	pchsource "src/r2pch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs {"%{prj.name}/src"}


	configuration { "macosx" }
		buildoptions {"-F %{prj.location}/vendor/MacOSX/SDL2"}
    	linkoptions {"-F %{prj.location}/vendor/MacOSX/SDL2"}
		includedirs {"/vendor/MacOSX/SDL2/SDL2.framework/Headers"}
		links
		{
			"SDL2.framework"
		}

	filter "system:macosx"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			"R2_PLATFORM_MAC"
		}

		filter{"configurations:"}

		postbuildcommands
		{
		--	("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/Sandbox/\"")
		}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			"R2_PLATFORM_WINDOWS",
			"R2_BUILD_DLL"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/Sandbox/\"")
		}

	filter "system:linux"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			R2_PLATFORM_LINUX
		}

	filter "configurations:Debug"
		defines "R2_DEBUG"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "R2_RELEASE"
		runtime "Release"
		optimize "On"

	filter "configurations:Publish"
		defines "R2_PUBLISH"
		runtime "Release"
		optimize "On"
		

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	--staticruntme "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin_int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"r2engine/src"
	}

	links
	{
		"r2engine"
	}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			"R2_PLATFORM_WINDOWS"
		}

	filter "system:macosx"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			"R2_PLATFORM_MAC"
		}
		
	filter "system:linux"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			R2_PLATFORM_LINUX
		}

