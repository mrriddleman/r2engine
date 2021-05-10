project "cmdln"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"include/cmdln/*.h",
		"src/*.cpp"
	}

	includedirs
	{
		"include",
	}

	configurations
	{
		"Debug",
		"Release",
		"Publish"
	}

	filter "system:windows"
		flags {"MultiProcessorCompile"}

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