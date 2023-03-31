#include "r2pch.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Core/Assets/AssetWriters/LevelAssetWriter.h"
#include "r2/Game/Level/LevelData_generated.h"

namespace r2::asset
{

	LevelAssetWriter::LevelAssetWriter()
	{

	}

	LevelAssetWriter::~LevelAssetWriter()
	{

	}

	const char* LevelAssetWriter::GetPattern()
	{
		return flat::LevelDataExtension();
	}

	r2::asset::AssetType LevelAssetWriter::GetType() const
	{
		return EngineAssetType::LEVEL;
	}

	bool LevelAssetWriter::WriteAsset(AssetFile& assetFile, const Asset& asset, const void* rawBuffer, u32 rawSize, u32 offset)
	{
		R2_CHECK(assetFile.IsOpen(), "We should have an opened file. %s isn't open", assetFile.FilePath());

		const auto numberOfBytesWritten = assetFile.WriteRawAsset(asset, (const byte*)rawBuffer, rawSize, offset);

		return numberOfBytesWritten == rawSize;
	}

}