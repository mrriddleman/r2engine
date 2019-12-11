
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
includeDirs["glm"] = "r2engine/vendor/glm"
includeDirs["flatbuffers"] = "r2engine/vendor/flatbuffers/include"
includeDirs["miniz"] = "r2engine/vendor/miniz/include"
includeDirs["loguru"] = "r2engine/vendor/loguru/src"
includeDirs["imgui"] = "r2engine/vendor/imgui"
includeDirs["catch2"] = "r2engine/vendor/Catch2"
includeDirs["sha256"] = "r2engine/vendor/sha256"
includeDirs["stb"] = "r2engine/vendor/stb"

include "r2engine/vendor/glad"
include "r2engine/vendor/miniz"
include "r2engine/vendor/loguru"
include "r2engine/vendor/imgui"

project "r2engine"
	location "r2engine"
	kind "SharedLib"
	language "C++"
	cppdialect "C++17"
	exceptionhandling "Off"
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
		"%{prj.name}/vendor/sha256/*.cpp",
		"%{prj.name}/vendor/stb/*.h",
		"%{prj.name}/vendor/stb/*.cpp"
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
		"%{includeDirs.sha256}",
		"%{includeDirs.stb}"
	}

	links
	{
		"miniz",
		"loguru",
		"imgui"
	}

	defines 
	{
		'R2_ENGINE_ASSETS="'..os.realpath('.')..'/engine_assets/Flatbuffer_Data/"',
		'R2_ENGINE_DATA_PATH="'..os.realpath('.')..'/r2engine/data"',
		'R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH="'..os.realpath('.')..'/r2engine/data/flatbuffer_schemas"'
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
		xcodebuildsettings { ["CC"] = "/usr/local/Cellar/llvm/8.0.0_1/bin/clang", ["COMPILER_INDEX_STORE_ENABLE"] = "No"}

		defines
		{
			"R2_PLATFORM_MAC",
			'R2_FLATC="'..os.realpath('.')..'/%{prj.name}/vendor/flatbuffers/bin/MacOSX/flatc"',
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
			"/usr/local/Cellar/llvm/8.0.0_1/include/c++/v1",
			
		}

		sysincludedirs
		{
			"%{prj.name}/vendor/assimp/MacOSX/include"
		}

		libdirs 
		{
			"%{prj.name}/vendor/FMOD/MacOSX/core/lib",
			"%{prj.name}/vendor/assimp/MacOSX/lib",
			"/usr/local/opt/llvm/lib"
		}

		links
		{
			"glad",
			"SDL2.framework",
			"OpenGL.framework",
			"c++fs"
		}

		filter "configurations:Debug"
			links
			{
				"fmodL",
				"assimp"
			}
		filter "configurations:not Debug"
			links
			{
				"fmod",
				"assimp"
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
		defines  {"R2_DEBUG", "R2_ASSET_PIPELINE"}
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
	exceptionhandling "Off"


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

		xcodebuildsettings { ["CC"] = "/usr/local/Cellar/llvm/8.0.0_1/bin/clang", ["COMPILER_INDEX_STORE_ENABLE"] = "No"}
		defines
		{
			"R2_PLATFORM_MAC"
		}

		filter "configurations:Debug"
			postbuildcommands 
			{
				"{COPY} ../r2engine/vendor/FMOD/MacOSX/core/lib/libfmodL.dylib ../bin/Debug_macosx_x86_64/%{prj.name}",
				"{COPY} ../r2engine/vendor/assimp/MacOSX/lib/libassimp.dylib ../bin/Debug_macosx_x86_64/%{prj.name}"
			}

		filter "configurations:Release"
			postbuildcommands 
			{
				"{COPY} ../r2engine/vendor/FMOD/MacOSX/core/lib/libfmod.dylib ../bin/Release_macosx_x86_64/%{prj.name}",
				"{COPY} ../r2engine/vendor/assimp/MacOSX/lib/libassimp.dylib ../bin/Release_macosx_x86_64/%{prj.name}"
			}  

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp" 
	language "C++"
	cppdialect "C++17"
	exceptionhandling "Off"

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
		"%{includeDirs.glm}",
		"%{includeDirs.flatbuffers}"
	}

	links
	{
		"r2engine"
	}

	defines 
	{
		'MANIFESTS_DIR="'..os.realpath('.')..'/%{prj.name}/assets/asset_manifests"',
		'ASSET_DIR="'..os.realpath('.')..'/%{prj.name}/assets"',
		'ASSET_BIN_DIR="'..os.realpath('.')..'/%{prj.name}/assets/asset_bin"',
		'ASSET_RAW_DIR="'..os.realpath('.')..'/%{prj.name}/assets/asset_raw"',
		'ASSET_FBS_SCHEMA_DIR="'..os.realpath('.')..'/%{prj.name}/assets/asset_fbs_schemas"',
		'ASSET_TEMP_DIR="'..os.realpath('.')..'/%{prj.name}/assets/asset_temp"',
		'APP_DIR="'..os.realpath('.')..'/%{prj.name}"',
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"R2_PLATFORM_WINDOWS"
		}

	filter "system:macosx"
		systemversion "latest"

		xcodebuildsettings { ["CC"] = "/usr/local/Cellar/llvm/8.0.0_1/bin/clang", ["COMPILER_INDEX_STORE_ENABLE"] = "No"}
		defines
		{
			"R2_PLATFORM_MAC"
		}

		filter "configurations:Debug"
			postbuildcommands 
			{
				"{COPY} ../r2engine/vendor/FMOD/MacOSX/core/lib/libfmodL.dylib ../bin/Debug_macosx_x86_64/%{prj.name}",
				"{COPY} ../r2engine/vendor/assimp/MacOSX/lib/libassimp.dylib ../bin/Debug_macosx_x86_64/%{prj.name}",
				"{RMDIR} ../bin/Debug_macosx_x86_64/%{prj.name}/sounds",
				"{COPY} ../engine_assets/sounds/ ../bin/Debug_macosx_x86_64/%{prj.name}"
			}

		filter "configurations:Release"
			postbuildcommands 
			{
				"{COPY} ../r2engine/vendor/FMOD/MacOSX/core/lib/libfmod.dylib ../bin/Release_macosx_x86_64/%{prj.name}",
				"{COPY} ../r2engine/vendor/assimp/MacOSX/lib/libassimp.dylib ../bin/Release_macosx_x86_64/%{prj.name}",
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
		defines {"R2_DEBUG", "R2_ASSET_PIPELINE"}
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

