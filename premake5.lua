
local ROOT = "../"    -- Path to project root

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

includeDirs = {}
includeDirs["glm"] = "%{prj.name}/vendor/glm"
includeDirs["flatbuffers"] = "%{prj.name}/vendor/flatbuffers/include"
includeDirs["miniz"] = "%{prj.name}/vendor/miniz/include"
includeDirs["loguru"] = "%{prj.name}/vendor/loguru/src"
includeDirs["imgui"] = "%{prj.name}/vendor/imgui"
includeDirs["catch2"] = "r2engine/vendor/Catch2"
includeDirs["sha256"] = "%{prj.nam}/vendor/sha256"

include "r2engine/vendor/glad"
include "r2engine/vendor/miniz"
include "r2engine/vendor/loguru"
include "r2engine/vendor/imgui"

project "r2engine"
	location "r2engine"
	kind "SharedLib"
	language "C++"
	cppdialect "C++17"
	--staticruntime "off"

	local output_dir_root	= "bin/" .. outputdir .. "/%{prj.name}"
	local obj_dir_root = "bin_int/" .. outputdir .. "/%{prj.name}"

	targetdir (output_dir_root)
	objdir (obj_dir_root)

	pchheader "src/r2pch.h"
	pchsource "src/r2pch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
		"%{prj.name}/vendor/Catch2/catch/**.hpp",
		"%{prj.name}/vendor/sha256/*.h",
		"%{prj.name}/vendor/sha256/*.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{includeDirs.glm}",
		"%{includeDirs.flatbuffers}",
		"%{includeDirs.miniz}",
		"%{includeDirs.loguru}",
		"%{includeDirs.imgui}",
		"%{includeDirs.catch2}",
		"%{includeDirs.sha256}"
	}

	links
	{
		"miniz",
		"loguru",
		"imgui"
	}

--[[
local CWD       = "cd " .. os.getcwd() .. "; " -- We are in current working directory
    local MKDIR     = "mkdir -p "
    local COPY      = "cp -rf "

    local SEPARATOR = "/"

    if(os.get() == "windows") then
      CWD      = "chdir " .. os.getcwd() .. " && "
      MKDIR     = "mkdir "
      COPY      = "xcopy /Q /E /Y /I "
      SEPARATOR = "\\"
    end
]]
	
	filter "system:macosx"
		systemversion "latest"

		defines
		{
			"R2_PLATFORM_MAC"
		}

		buildoptions 
		{
			"-F %{prj.location}/vendor/SDL2/MacOSX",
		}
    	
    	linkoptions
    	{
    		"-F %{prj.location}/vendor/SDL2/MacOSX"
    	}

		includedirs 
		{
			"%{prj.name}/vendor/glad/MacOSX/include",
			"%{prj.name}/vendor/FMOD/MacOSX/core/inc",
			--"%{prj.name}/vendor/FMOD/MacOSX/studio/inc"
		}

		libdirs 
		{
			"%{prj.name}/vendor/FMOD/MacOSX/core/lib",
			--"%{prj.name}/vendor/FMOD/MacOSX/studio/lib"
		}

		links
		{
			"glad",
			"SDL2.framework",
			"OpenGL.framework",
		}

		filter "configurations:Debug"
			links
			{
				--//"fmodstudioL",
				"fmodL"
			}
		filter "configurations:not Debug"
			links
			{
				"fmod"
				--"fmodstudio"
			}


	filter "system:windows"
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
		systemversion "latest"

		defines
		{
			"R2_PLATFORM_LINUX"
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

		

project "r2Tests"
	location "r2Tests"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin_int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{includeDirs.catch2}",
		"r2engine/src"
	}

	links
	{
		"r2engine"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"R2_PLATFORM_WINDOWS"
		}

	filter "system:macosx"
		systemversion "latest"

		defines
		{
			"R2_PLATFORM_MAC"
		}


project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp" 
	language "C++"
	cppdialect "C++17"
	--staticruntme "off"

	local sandboxOutputDir = "bin/" .. outputdir .. "/%{prj.name}"

	targetdir (sandboxOutputDir)
	objdir ("bin_int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"r2engine/src",
		"%{includeDirs.glm}"
	}

	links
	{
		"r2engine"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"R2_PLATFORM_WINDOWS"
		}

	filter "system:macosx"
		systemversion "latest"

		defines
		{
			"R2_PLATFORM_MAC"
		}

		filter "configurations:Debug"
			postbuildcommands 
			{
				"{COPY} ../r2engine/vendor/FMOD/MacOSX/core/lib/libfmodL.dylib ../bin/Debug_macosx_x86_64/%{prj.name}",
				"{RMDIR} ../bin/Debug_macosx_x86_64/%{prj.name}/sounds",
				"{COPY} ../engine_assets/sounds/ ../bin/Debug_macosx_x86_64/%{prj.name}"
			}

		filter "configurations:Release"
			postbuildcommands 
			{
				"{COPY} ../r2engine/vendor/FMOD/MacOSX/core/lib/libfmod.dylib ../bin/Release_macosx_x86_64/%{prj.name}",
				"{RMDIR} ../bin/Release_macosx_x86_64/%{prj.name}/sounds",
				"{COPY} ../engine_assets/sounds ./bin/Release_macosx_x86_64/%{prj.name}/sounds"
			}    
		
	filter "system:linux"
		systemversion "latest"

		defines
		{
			"R2_PLATFORM_LINUX"
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

