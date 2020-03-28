project "glad"
	kind "StaticLib"
	language "C"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	filter "system:macosx"
		files
		{
			"MacOSX/include/glad/glad.h",
			"MacOSX/include/KHR/khrplatform.h",
			"MacOSX/src/glad.c"
		}

		includedirs
		{
			"MacOSX/include"
		}

	filter "system:windows"
		files
		{
			"Windows/include/glad/glad.h",
			"Windows/include/KHR/khrplatform.h",
			"Windows/src/glad.c"
		}

		includedirs
		{
			"Windows/include"
		}

	
	configurations
	{
		"Debug",
		"Release",
		"Publish"
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