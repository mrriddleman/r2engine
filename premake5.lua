
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
includeDirs["cgltf"] = "r2engine/vendor/cgltf"
includeDirs["assetlib"] = "r2engine/libs/assetlib/include"
includeDirs["texturemetadata"] = "r2engine/libs/assetlib/include/assetlib"
includeDirs["ImGuiFileDialog"] = "r2engine/vendor/ImGuiFileDialog"

include "r2engine/vendor/glad"
include "r2engine/vendor/miniz"
include "r2engine/vendor/loguru"
include "r2engine/vendor/imgui"
include "r2engine/libs/assetlib"
include "r2engine/libs/ibl"
include "r2engine/libs/cmdln"
include "r2engine/tools/cubemapgen"
include "r2engine/tools/assetconverter"

project "r2engine"
	location "r2engine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	exceptionhandling "Off"
	disablewarnings {"26812"}
    linkoptions { "-IGNORE:4006" }

	local output_dir_root	= "bin/" .. outputdir .. "/%{prj.name}"
	local obj_dir_root = "bin_int/" .. outputdir .. "/%{prj.name}"

	targetdir (output_dir_root)
	objdir (obj_dir_root)
	
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
		"%{prj.name}/vendor/stb/*.cpp",
		"%{prj.name}/vendor/cgltf/*.h",
		"%{prj.name}/vendor/cgltf/*.cpp",
		"%{prj.name}/vendor/ImGuiFileDialog/*.h",
		"%{prj.name}/vendor/ImGuiFileDialog/*.cpp",
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
		"%{includeDirs.stb}",
		"%{includeDirs.cgltf}",
		"%{includeDirs.assetlib}",
		"%{includeDirs.texturemetadata}",
		"%{includeDirs.ImGuiFileDialog}",
		"%{prj.name}/src/r2/Render/Model",
		"%{prj.name}/src/r2/Game/ECS"
	}

	links
	{
		"miniz",
		"loguru",
		"imgui",
		"assetlib"
	}

	defines 
	{
		'R2_ENGINE_ASSETS="'..os.getcwd()..'/engine_assets/Flatbuffer_Data/"',
		'R2_ENGINE_DATA_PATH="'..os.getcwd()..'/r2engine/data"',
		'R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH="'..os.getcwd()..'/r2engine/data/flatbuffer_schemas"',
		'R2_ENGINE_ASSET_PATH="'..os.getcwd()..'/r2engine/assets"',
		'R2_ENGINE_ASSET_BIN_PATH="'..os.getcwd()..'/r2engine/assets_bin"',
		
		'R2_ENGINE_INTERNAL_SHADERS_RAW_DIR="'..os.getcwd()..'/r2engine/assets/shaders/raw"',
		'R2_ENGINE_INTERNAL_SHADERS_MANIFESTS_DIR="'..os.getcwd()..'/r2engine/assets_bin/shaders/manifests"',
		'R2_ENGINE_INTERNAL_SHADER_PASSES_RAW_DIR="'..os.getcwd()..'/r2engine/assets/shaders/manifests/internal_passes"',
		'R2_ENGINE_INTERNAL_SHADER_PASSES_BIN_DIR="'..os.getcwd()..'/r2engine/assets_bin/shaders/manifests/internal_passes"',

		'R2_ENGINE_INTERNAL_MATERIALS_DIR_BIN="'..os.getcwd()..'/r2engine/assets_bin/materials"',
		'R2_ENGINE_INTERNAL_MATERIALS_DIR="'..os.getcwd()..'/r2engine/assets/materials/packs"',
		'R2_ENGINE_INTERNAL_MATERIALS_PACKS_DIR_BIN="'..os.getcwd()..'/r2engine/assets_bin/materials/packs"',
		'R2_ENGINE_INTERNAL_MATERIALS_MANIFESTS="'..os.getcwd()..'/r2engine/assets/materials/manifests"',
		'R2_ENGINE_INTERNAL_MATERIALS_MANIFESTS_BIN="'..os.getcwd()..'/r2engine/assets_bin/materials/manifests"',
		
		'R2_ENGINE_INTERNAL_MATERIALS_PARAMS_PACKS_DIR="'..os.getcwd()..'/r2engine/assets/materials/params_packs"',
		'R2_ENGINE_INTERNAL_MATERIALS_PARAMS_PACKS_DIR_BIN="'..os.getcwd()..'/r2engine/assets_bin/materials/params_packs"',

		'R2_ENGINE_INTERNAL_TEXTURES_DIR="'..os.getcwd()..'/r2engine/assets/textures/packs"',
		'R2_ENGINE_INTERNAL_TEXTURES_MANIFESTS="'..os.getcwd()..'/r2engine/assets/textures/manifests"',
		'R2_ENGINE_INTERNAL_TEXTURES_BIN="'..os.getcwd()..'/r2engine/assets_bin/textures"',
		'R2_ENGINE_INTERNAL_TEXTURES_MANIFESTS_BIN="'..os.getcwd()..'/r2engine/assets_bin/textures/manifests"',
		'R2_ENGINE_INTERNAL_TEXTURES_DIR_BIN="'..os.getcwd()..'/r2engine/assets_bin/textures/packs"',

		'R2_ENGINE_INTERNAL_MODELS_RAW="'..os.getcwd()..'/r2engine/assets/models/raw"',
		'R2_ENGINE_INTERNAL_MODELS_BIN="'..os.getcwd()..'/r2engine/assets_bin/models/"',

		'R2_ENGINE_ASSET_CONVERTER_EXE="'..os.getcwd()..'/r2engine/tools/assetconverter/bin/Publish_windows_x86_64/assetconverter/assetconverter.exe"',

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
		pchheader "src/r2pch.h"

		defines
		{
			"R2_PLATFORM_MAC",
			'R2_FLATC="'..os.getcwd()..'/%{prj.name}/vendor/flatbuffers/bin/MacOSX/flatc"',
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

		filter {"configurations:Debug", "system:macosx"}
			links
			{
				"fmodL",
				"assimp"
			}
		filter {"configurations:not Debug", "system:macosx"}
			links
			{
				"fmod",
				"assimp"
			}


	filter "system:windows"
		systemversion "latest"
    	pchheader "r2pch.h"
    	pchsource "r2engine/src/r2pch.cpp"
    	flags {"MultiProcessorCompile"}


		libdirs
		{
			"%{prj.name}/vendor/SDL2/Windows/lib/x64",
			"%{prj.name}/vendor/SDL2Image/Windows/lib/x64"
		}

		defines
		{
			"R2_PLATFORM_WINDOWS",
			"R2_BUILD_DLL",
			'R2_FLATC="'..os.getcwd()..'/%{prj.name}/vendor/flatbuffers/bin/Windows/flatc.exe"',
		}

		includedirs
		{
			"%{prj.name}/vendor/SDL2/Windows/include",
			"%{prj.name}/vendor/glad/Windows/include",
			"%{prj.name}/vendor/FMOD/Windows/core/inc",
			"%{prj.name}/vendor/SDL2Image/Windows/include"
		}

		sysincludedirs
		{
			"%{prj.name}/vendor/assimp/Windows/include"
		}

		links
		{
			"SDL2",
			"SDL2main",
			"glad",
			"SDL2_image",
			"flatbuffers"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/Sandbox/\"")
		}

	filter {"system:windows", "configurations:Release"}
		libdirs
		{
			"%{prj.name}/vendor/flatbuffers/lib/Windows/Release/x64",
		}

	filter {"system:windows", "configurations:Publish"}
		libdirs
		{
			"%{prj.name}/vendor/flatbuffers/lib/Windows/Release/x64",
		}

	filter {"system:windows", "configurations:Debug"}
		libdirs
		{
			"%{prj.name}/vendor/flatbuffers/lib/Windows/Debug/x64"
		}


	filter "files:r2engine/vendor/stb/stb_image_impl.cpp or files:r2engine/vendor/stb/stb_image_resize_impl.cpp or files:r2engine/vendor/stb/stb_image_write_impl.cpp or files:r2engine/vendor/sha256/sha256.cpp"
		flags {"NoPCH"}

	filter "files:r2engine/vendor/cgltf/cgltf.cpp or files:r2engine/vendor/cgltf/cgltf_write.cpp"
		flags {"NoPCH"}

	filter "files:r2engine/vendor/ImGuiFileDialog/ImGuiFileDialog.cpp"
		flags {"NoPCH"}


	filter "system:linux"
		systemversion "latest"

		defines
		{
			"R2_PLATFORM_LINUX"
		}

	filter "configurations:Debug"
		defines  {"R2_DEBUG", "R2_ASSET_PIPELINE", "R2_ASSET_CACHE_DEBUG", "R2_EDITOR", "R2_IMGUI"}
		runtime "Debug"
		symbols "On"
		optimize "Off"
		staticruntime "off"

	filter "configurations:Release"
		defines "R2_RELEASE"
		runtime "Release"
		symbols "Off"
		optimize "On"
		staticruntime "off"

	filter "configurations:Publish"
		defines "R2_PUBLISH"
		runtime "Release"
		symbols "Off"
		optimize "Full"
		staticruntime "off"

		

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
		"%{includeDirs.glm}",
		"r2engine/src"
	}

	links
	{
		"r2engine"
	}

	filter "system:windows"
		systemversion "latest"
		flags {"MultiProcessorCompile"}
		
		defines
		{
			"R2_PLATFORM_WINDOWS"
		}

		libdirs
		{
			"r2engine/vendor/FMOD/Windows/core/lib/x64",
		}


	filter {"configurations:Debug", "system:windows"}

		libdirs
		{
			"r2engine/vendor/assimp/Windows/lib/Debug"
		}

		links
		{
			"fmodL_vc",
			"assimp-vc142-mtd"
		}

		postbuildcommands
		{
			"{COPY} ../r2engine/vendor/SDL2/Windows/lib/x64/SDL2.dll ../bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/FMOD/Windows/core/lib/x64/fmodL.dll ../bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/assimp/Windows/lib/Debug/assimp-vc142-mtd.dll ../bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/SDL2_image.dll ../bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libtiff-5.dll ../bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libpng16-16.dll ../bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libjpeg-9.dll ../bin/Debug_windows_x86_64/%{prj.name}"
		}

	filter {"configurations:Release", "system:windows"}

		libdirs
		{
			"r2engine/vendor/assimp/Windows/lib/Release",
		}

		links
		{
			"fmod_vc",
			"assimp-vc142-mt"
		}

		postbuildcommands
		{
			"{COPY} ../r2engine/vendor/SDL2/Windows/lib/x64/SDL2.dll ../bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/FMOD/Windows/core/lib/x64/fmod.dll ../bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/assimp/Windows/lib/Release/assimp-vc142-mt.dll ../bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/SDL2_image.dll ../bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libtiff-5.dll ../bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libpng16-16.dll ../bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libjpeg-9.dll ../bin/Release_windows_x86_64/%{prj.name}"
		}

	filter "system:macosx"
		systemversion "latest"

		xcodebuildsettings { ["CC"] = "/usr/local/Cellar/llvm/8.0.0_1/bin/clang", ["COMPILER_INDEX_STORE_ENABLE"] = "No"}
		defines
		{
			"R2_PLATFORM_MAC"
		}

	filter {"configurations:Debug", "system:macosx"}
		postbuildcommands 
		{
			"{COPY} ../r2engine/vendor/FMOD/MacOSX/core/lib/libfmodL.dylib ../bin/Debug_macosx_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/assimp/MacOSX/lib/libassimp.dylib ../bin/Debug_macosx_x86_64/%{prj.name}"
		}

	filter {"configurations:Release", "system:macosx"}
		postbuildcommands 
		{
			"{COPY} ../r2engine/vendor/FMOD/MacOSX/core/lib/libfmod.dylib ../bin/Release_macosx_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/assimp/MacOSX/lib/libassimp.dylib ../bin/Release_macosx_x86_64/%{prj.name}"
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

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp" 
	language "C++"
	cppdialect "C++17"
	exceptionhandling "Off"
	
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
		"%{includeDirs.flatbuffers}",
		"%{includeDirs.texturemetadata}",
		"%{includeDirs.assetlib}"
	}

	links
	{
		"r2engine"
	}

	defines 
	{
		'MANIFESTS_DIR="'..os.getcwd()..'/%{prj.name}/assets/asset_manifests"',
		'ASSET_DIR="'..os.getcwd()..'/%{prj.name}/assets"',
		'ASSET_BIN_DIR="'..os.getcwd()..'/%{prj.name}/assets/asset_bin"',
		'ASSET_RAW_DIR="'..os.getcwd()..'/%{prj.name}/assets/asset_raw"',
		'ASSET_FBS_SCHEMA_DIR="'..os.getcwd()..'/%{prj.name}/assets/asset_fbs_schemas"',
		'ASSET_TEMP_DIR="'..os.getcwd()..'/%{prj.name}/assets/asset_temp"',
		'APP_DIR="'..os.getcwd()..'/%{prj.name}"',

		'SANDBOX_MATERIALS_DIR_BIN="'..os.getcwd()..'/%{prj.name}/assets_bin/Sandbox_Materials"',
		'SANDBOX_MATERIALS_DIR="'..os.getcwd()..'/%{prj.name}/assets/Sandbox_Materials/packs"',
		'SANDBOX_MATERIALS_PACKS_DIR_BIN="'..os.getcwd()..'/%{prj.name}/assets_bin/Sandbox_Materials/packs"',
		'SANDBOX_MATERIALS_MANIFESTS="'..os.getcwd()..'/%{prj.name}/assets/Sandbox_Materials/manifests"',
		'SANDBOX_MATERIALS_MANIFESTS_BIN="'..os.getcwd()..'/%{prj.name}/assets_bin/Sandbox_Materials/manifests"',

		'SANDBOX_MATERIAL_PARAMS_DIR="'..os.getcwd()..'/%{prj.name}/assets/Sandbox_Materials/params_packs"',
		'SANDBOX_MATERIALS_PARAMS_DIR_BIN="'..os.getcwd()..'/%{prj.name}/assets_bin/Sandbox_Materials/params_packs"',
		
		'SANDBOX_TEXTURES_DIR="'..os.getcwd()..'/%{prj.name}/assets/Sandbox_Textures/packs"',
		'SANDBOX_TEXTURES_MANIFESTS="'..os.getcwd()..'/%{prj.name}/assets/Sandbox_Textures/manifests"',
		'SANDBOX_TEXTURES_BIN="'..os.getcwd()..'/%{prj.name}/assets_bin/Sandbox_Textures"',
		'SANDBOX_TEXTURES_MANIFESTS_BIN="'..os.getcwd()..'/%{prj.name}/assets_bin/Sandbox_Textures/manifests"',
		'SANDBOX_TEXTURES_PACKS_BIN="'..os.getcwd()..'/%{prj.name}/assets_bin/Sandbox_Textures/packs"',
		'SANDBOX_TEXTURES_MANIFESTS_BIN="'..os.getcwd()..'/%{prj.name}/assets_bin/Sandbox_Textures/manifests"',

		'SANDBOX_MODELS_DIR_BIN="'..os.getcwd()..'/%{prj.name}/assets_bin/Sandbox_Models"',
		'SANDBOX_MODELS_DIR_RAW="'..os.getcwd()..'/%{prj.name}/assets/Sandbox_Models"',

		'SANDBOX_ANIMATIONS_DIR_BIN="'..os.getcwd()..'/%{prj.name}/assets_bin/Sandbox_Animations"',
		'SANDBOX_ANIMATIONS_DIR_RAW="'..os.getcwd()..'/%{prj.name}/assets/Sandbox_Animations"',

		'SANDBOX_SHADERS_DIR_BIN="'..os.getcwd()..'/%{prj.name}/assets_bin/Sandbox_Shaders"',
		'SANDBOX_SHADERS_INTERNAL_SHADER_DIR_BIN="'..os.getcwd()..'/%{prj.name}/assets_bin/Sandbox_Shaders/manifests/internal_passes"',
		'SANDBOX_SHADERS_DIR_RAW="'..os.getcwd()..'/%{prj.name}/assets/shaders"',
		'SANDBOX_SHADERS_INTERNAL_SHADER_DIR_RAW="'..os.getcwd()..'/%{prj.name}/assets/shaders/manifests/internal_passes"',

		'SANDBOX_LEVELS_DIR_BIN="'..os.getcwd()..'/%{prj.name}/assets_bin/Sandbox_Levels"',
		'SANDBOX_LEVELS_DIR_RAW="'..os.getcwd()..'/%{prj.name}/assets/Sandbox_Levels"'
	}

	filter "system:windows"
		systemversion "latest"
		flags {"MultiProcessorCompile"}
		buildoptions{"/STACK:reserve=2097152"}
		libdirs
		{
			"r2engine/vendor/FMOD/Windows/core/lib/x64",
		}

		defines
		{
			"R2_PLATFORM_WINDOWS"
		}

	filter {"configurations:Debug", "system:windows"}
		libdirs
		{
			"r2engine/vendor/assimp/Windows/lib/Debug"
		}

		links
		{
			"fmodL_vc",
			"assimp-vc142-mtd"
		}

		postbuildcommands 
		{
			"{COPY} ../r2engine/vendor/SDL2/Windows/lib/x64/SDL2.dll ../bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/FMOD/Windows/core/lib/x64/fmodL.dll ../bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/assimp/Windows/lib/Debug/assimp-vc142-mtd.dll ../bin/Debug_windows_x86_64/%{prj.name}",
			"{RMDIR} ../bin/Debug_windows_x86_64/%{prj.name}/sounds",
			"{COPY} ../engine_assets/sounds/ ../bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/SDL2_image.dll ../bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libtiff-5.dll ../bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libpng16-16.dll ../bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libjpeg-9.dll ../bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/zlib1.dll ../bin/Debug_windows_x86_64/%{prj.name}"
		}

	filter {"configurations:Release", "system:windows"}

		libdirs
		{
			"r2engine/vendor/assimp/Windows/lib/Release",
		}

		links
		{
			"fmod_vc",
			"assimp-vc142-mt"
		}
		postbuildcommands 
		{
			"{COPY} ../r2engine/vendor/SDL2/Windows/lib/x64/SDL2.dll ../bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/FMOD/Windows/core/lib/x64/fmod.dll ../bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/assimp/Windows/lib/Release/assimp-vc142-mt.dll ../bin/Release_windows_x86_64/%{prj.name}",
			"{RMDIR} ../bin/Release_windows_x86_64/%{prj.name}/sounds",
			"{COPY} ../engine_assets/sounds ./bin/Release_windows_x86_64/%{prj.name}/sounds",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/SDL2_image.dll ../bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libtiff-5.dll ../bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libpng16-16.dll ../bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libjpeg-9.dll ../bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/zlib1.dll ../bin/Release_windows_x86_64/%{prj.name}"
		} 

	filter {"configurations:Publish", "system:windows"}
		libdirs
		{
			"r2engine/vendor/assimp/Windows/lib/Release",
		}

		links
		{
			"fmod_vc",
			"assimp-vc142-mt"
		}

		postbuildcommands 
		{
			"{COPY} ../r2engine/vendor/SDL2/Windows/lib/x64/SDL2.dll ../bin/Publish_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/FMOD/Windows/core/lib/x64/fmod.dll ../bin/Publish_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/assimp/Windows/lib/Release/assimp-vc142-mt.dll ../bin/Publish_windows_x86_64/%{prj.name}",
			"{RMDIR} ../bin/Publish_windows_x86_64/%{prj.name}/sounds",
			"{COPY} ../engine_assets/sounds ./bin/Publish_windows_x86_64/%{prj.name}/sounds",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/SDL2_image.dll ../bin/Publish_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libtiff-5.dll ../bin/Publish_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libpng16-16.dll ../bin/Publish_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/libjpeg-9.dll ../bin/Publish_windows_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/SDL2Image/Windows/lib/x64/zlib1.dll ../bin/Publish_windows_x86_64/%{prj.name}"
		}

	filter "system:macosx"
		systemversion "latest"

		xcodebuildsettings { ["CC"] = "/usr/local/Cellar/llvm/8.0.0_1/bin/clang", ["COMPILER_INDEX_STORE_ENABLE"] = "No"}
		defines
		{
			"R2_PLATFORM_MAC"
		}

	filter {"configurations:Debug", "system:macosx"}
		postbuildcommands 
		{
			"{COPY} ../r2engine/vendor/FMOD/MacOSX/core/lib/libfmodL.dylib ../bin/Debug_macosx_x86_64/%{prj.name}",
			"{COPY} ../r2engine/vendor/assimp/MacOSX/lib/libassimp.dylib ../bin/Debug_macosx_x86_64/%{prj.name}",
			"{RMDIR} ../bin/Debug_macosx_x86_64/%{prj.name}/sounds",
			"{COPY} ../engine_assets/sounds/ ../bin/Debug_macosx_x86_64/%{prj.name}"
		}

	filter {"configurations:Release", "system:macosx"}
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
		defines {"R2_DEBUG", "R2_ASSET_PIPELINE", "R2_ASSET_CACHE_DEBUG", "R2_EDITOR", "R2_IMGUI"}
		runtime "Debug"
		symbols "On"
		staticruntime "off"

	filter "configurations:Release"
		defines "R2_RELEASE"
		runtime "Release"
		optimize "On"
		staticruntime "off"

	filter "configurations:Publish"
		defines "R2_PUBLISH"
		runtime "Release"
		optimize "Full"
		staticruntime "off"