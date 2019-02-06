project "glad"
	kind "StaticLib"
	language "C"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name")

	configuration "macosx"
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