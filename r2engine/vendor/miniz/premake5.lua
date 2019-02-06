project "miniz"
	kind "StaticLib"
	language "C"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name")

	configuration "macosx"
		files
		{
			"include/miniz.h",
			"src/miniz.c"
		}

		includedirs
		{
			"include"
		}