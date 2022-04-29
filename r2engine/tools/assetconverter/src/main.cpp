#include <iostream>

#include "assetlib/AssetFile.h"
#include "assetlib/TextureAsset.h"
#include "assetlib/TextureMetaData_generated.h"

#include "cmdln/CommandLine.h"

#include "flatbuffers/flatbuffers.h"

#include "stb_image.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <filesystem>
#include <string>

#include "nvtt/nvtt.h"

namespace fs = std::filesystem;

struct Arguments
{
	std::string inputDir;
	std::string outputDir;
};

int main(int agrc, char** argv)
{
	Arguments arguments;
	r2::cmdln::CommandLine args("Asset Converter will convert raw assets into assets r2 can use");

	std::cout << "Hello Converter" << std::endl;
	return 0;
}