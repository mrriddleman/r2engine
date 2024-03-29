#ifndef __ASSET_LIB_MODEL_CONVERT_H__
#define __ASSET_LIB_MODEL_CONVERT_H__

#include <filesystem>

namespace r2::assets::assetlib
{
	bool ConvertModel(
		const std::filesystem::path& inputFilePath,
		const std::filesystem::path& parentOutputDir,
		const std::filesystem::path& binaryMaterialParamPacksManifestFile,
		const std::filesystem::path& rawMaterialsParentDirectory,
		const std::filesystem::path& engineTexturePacksManifestFile,
		const std::filesystem::path& texturePacksManifestFile,
		bool forceRebuild,
		uint32_t numAnimationSamples);
}

#endif