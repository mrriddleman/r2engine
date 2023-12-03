
assetconverterIncludeDirs = {}
assetconverterIncludeDirs["assetlib"] = "../../libs/assetlib/include"
assetconverterIncludeDirs["glm"] = "../../vendor/glm"
assetconverterIncludeDirs["cmdln"] = "../../libs/cmdln/include"
assetconverterIncludeDirs["stb"] = "../../vendor/stb"
assetconverterIncludeDirs["flatbuffers"] = "../../vendor/flatbuffers/include"
assetconverterIncludeDirs["nvtt"] = "./libs/nvtt/include"

project "assetconverter"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	exceptionhandling "Off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin_int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"include/**.h",
		"src/**.cpp",
	}

	includedirs
	{
		"include",
		"../../src/r2/Render/Model/Textures/",
		"../../src/r2/Render/Model/Shader/",
		"../../src/r2/Render/Model/Materials/",
		"../../src/r2/Core/Assets/",
		"%{assetconverterIncludeDirs.assetlib}",
		"%{assetconverterIncludeDirs.glm}",
		"%{assetconverterIncludeDirs.cmdln}",
		"%{assetconverterIncludeDirs.stb}",
		"%{assetconverterIncludeDirs.flatbuffers}",
		"%{assetconverterIncludeDirs.nvtt}"
	}

	sysincludedirs
	{
		"../../vendor/assimp/Windows/include"
	}

	configurations
	{
		"Debug",
		"Release",
		"Publish"
	}

	libdirs
	{
		"libs/nvtt/lib/x64-v142"
	}

	links
	{
		"assetlib",
		"cmdln",
		"nvtt30106"
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

		libdirs
		{
			"../../vendor/assimp/Windows/lib/Debug"
		}

		links
		{
			"assimp-vc142-mtd"
		}

		postbuildcommands
		{
			"{COPY} ../../vendor/assimp/Windows/lib/Debug/assimp-vc142-mtd.dll ./bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ./libs/nvtt/nvtt30106.dll ./bin/Debug_windows_x86_64/%{prj.name}",
			"{COPY} ./libs/nvtt/cudart64_110.dll ./bin/Debug_windows_x86_64/%{prj.name}"
		}

	filter "configurations:Release"
		runtime "Release"
		symbols "Off"
		optimize "On"
		staticruntime "off"

		libdirs
		{
			"../../vendor/assimp/Windows/lib/Release"
		}

		links
		{
			"assimp-vc142-mt"
		}

		postbuildcommands
		{
			"{COPY} ../../vendor/assimp/Windows/lib/Release/assimp-vc142-mt.dll ./bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ./libs/nvtt/nvtt30106.dll ./bin/Release_windows_x86_64/%{prj.name}",
			"{COPY} ./libs/nvtt/cudart64_110.dll ./bin/Release_windows_x86_64/%{prj.name}"
		}


	filter "configurations:Publish"
		runtime "Release"
		symbols "Off"
		optimize "Full"
		staticruntime "off"

		libdirs
		{
			"../../vendor/assimp/Windows/lib/Release"
		}

		links
		{
			"assimp-vc142-mt"
		}

		postbuildcommands
		{
			"{COPY} ../../vendor/assimp/Windows/lib/Release/assimp-vc142-mt.dll ./bin/Publish_windows_x86_64/%{prj.name}",
			"{COPY} ./libs/nvtt/nvtt30106.dll ./bin/Publish_windows_x86_64/%{prj.name}",
			"{COPY} ./libs/nvtt/cudart64_110.dll ./bin/Publish_windows_x86_64/%{prj.name}"
		}