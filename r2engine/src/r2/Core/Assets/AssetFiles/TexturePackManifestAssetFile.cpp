#include "r2pch.h"
#include "r2/Core/Assets/AssetFiles/TexturePackManifestAssetFile.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetBuffer.h"

namespace r2::asset
{

	TexturePackManifestAssetFile::TexturePackManifestAssetFile()
		:mTexturePacksManifest(nullptr)
	{

	}

	TexturePackManifestAssetFile::~TexturePackManifestAssetFile()
	{
	}

	bool TexturePackManifestAssetFile::Init(AssetCache* noptrAssetCache, const char* binPath, const char* rawPath, r2::asset::AssetType assetType)
	{
		mnoptrAssetCache = noptrAssetCache;
		mManifestAssetFile = (r2::asset::AssetFile*)r2::asset::lib::MakeRawAssetFile(binPath, r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(assetType));
		mAssetType = assetType;
		r2::util::PathCpy(mRawPath, rawPath);
		return mManifestAssetFile != nullptr;
	}

	void TexturePackManifestAssetFile::Shutdown()
	{
		if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord) ||
			!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle))
		{
			R2_CHECK(false, "We haven't unloaded the Manifest!");
			return;
		}	
	}

	r2::asset::AssetType TexturePackManifestAssetFile::GetAssetType() const
	{
		return mAssetType;
	}

	void AddAllTexturesFromTextureType(const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>* texturePaths, r2::asset::FileList fileList)
	{
		for (u32 i = 0; i < texturePaths->size(); ++i)
		{
			r2::asset::RawAssetFile* assetFile = r2::asset::lib::MakeRawAssetFile(texturePaths->Get(i)->c_str(), r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::TEXTURE));
			r2::sarr::Push(*fileList, (r2::asset::AssetFile*)assetFile);
		}
	}

	void AddAllTexturePathsInTexturePackToFileList(const flat::TexturePack* texturePack, r2::asset::FileList fileList)
	{
		AddAllTexturesFromTextureType(texturePack->albedo(), fileList);
		AddAllTexturesFromTextureType(texturePack->anisotropy(), fileList);
		AddAllTexturesFromTextureType(texturePack->clearCoat(), fileList);
		AddAllTexturesFromTextureType(texturePack->clearCoatNormal(), fileList);
		AddAllTexturesFromTextureType(texturePack->clearCoatRoughness(), fileList);
		AddAllTexturesFromTextureType(texturePack->detail(), fileList);
		AddAllTexturesFromTextureType(texturePack->emissive(), fileList);
		AddAllTexturesFromTextureType(texturePack->height(), fileList);
		AddAllTexturesFromTextureType(texturePack->metallic(), fileList);
		AddAllTexturesFromTextureType(texturePack->normal(), fileList);
		AddAllTexturesFromTextureType(texturePack->occlusion(), fileList);
		AddAllTexturesFromTextureType(texturePack->roughness(), fileList);

		if (texturePack->metaData() && texturePack->metaData()->mipLevels())
		{
			for (flatbuffers::uoffset_t i = 0; i < texturePack->metaData()->mipLevels()->size(); ++i)
			{
				const flat::MipLevel* mipLevel = texturePack->metaData()->mipLevels()->Get(i);

				for (flatbuffers::uoffset_t side = 0; side < mipLevel->sides()->size(); ++side)
				{
					r2::asset::RawAssetFile* assetFile = r2::asset::lib::MakeRawAssetFile(mipLevel->sides()->Get(side)->textureName()->c_str(), r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::CUBEMAP_TEXTURE));
					r2::sarr::Push(*fileList, (r2::asset::AssetFile*)assetFile);
				}
			}
		}
	}

	bool TexturePackManifestAssetFile::AddAllFilePaths(FileList fileList)
	{
		auto texturePacks = mTexturePacksManifest->texturePacks();

		for (flatbuffers::uoffset_t i = 0; i < texturePacks->size(); ++i)
		{
			const flat::TexturePack* texturePack = texturePacks->Get(i);

			AddAllTexturePathsInTexturePackToFileList(texturePack, fileList);
		}

		return true;
	}

	u64 TexturePackManifestAssetFile::GetManifestFileHandle() const
	{
		return r2::asset::GetAssetNameForFilePath(FilePath(), mAssetType);
	}

#ifdef R2_ASSET_PIPELINE
	bool TexturePackManifestAssetFile::ReloadFilePath(const std::vector<std::string>& paths, r2::asset::HotReloadType type)
	{
		return mReloadFilePathFunc(paths, FilePath(), GetManifestData(), type);
	}
#endif

	const char* TexturePackManifestAssetFile::FilePath() const
	{
		return mManifestAssetFile->FilePath();
	}

	bool TexturePackManifestAssetFile::LoadManifest()
	{
		if (mnoptrAssetCache == nullptr)
		{
			R2_CHECK(false, "Passed in nullptr for the AssetCache");
			return false;
		}

		const r2::asset::AssetFile* foundAssetFile = mnoptrAssetCache->GetAssetFileForAssetHandle({ mManifestAssetFile->GetAssetHandle(0), mnoptrAssetCache->GetSlot() });

		if (foundAssetFile != mManifestAssetFile)
		{
			//@Temporary(Serge): add it to the file list - remove when we do the AssetCache refactor
			FileList fileList = mnoptrAssetCache->GetFileList();
			r2::sarr::Push(*fileList, (AssetFile*)mManifestAssetFile);
		}

		mManifestAssetHandle = mnoptrAssetCache->LoadAsset(r2::asset::Asset::MakeAssetFromFilePath(FilePath(), mAssetType));

		R2_CHECK(!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle), "The assetHandle for %s is invalid!\n", FilePath());

		mManifestCacheRecord = mnoptrAssetCache->GetAssetBuffer(mManifestAssetHandle);

		R2_CHECK(!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord), "The asset cache record is empty");

		mTexturePacksManifest = flat::GetTexturePacksManifest(mManifestCacheRecord.GetAssetBuffer()->Data());

		R2_CHECK(mTexturePacksManifest != nullptr, "Should never happen");

		return true;
	}

	bool TexturePackManifestAssetFile::UnloadManifest()
	{
		if (r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			return true;
		}

		bool success = mnoptrAssetCache->ReturnAssetBuffer(mManifestCacheRecord);
		R2_CHECK(success, "Failed to return the asset cache record");

		mManifestCacheRecord = {};
		mManifestAssetHandle = {};
		mTexturePacksManifest = nullptr;

		return success;
	}

	const byte* TexturePackManifestAssetFile::GetManifestData() const
	{
		return mManifestCacheRecord.GetAssetBuffer()->Data();
	}

#ifdef R2_ASSET_PIPELINE
	bool TexturePackManifestAssetFile::SaveManifest()
	{
		TODO;
		return false;
	}

	void TexturePackManifestAssetFile::Reload()
	{
		if (!AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			mnoptrAssetCache->ReturnAssetBuffer(mManifestCacheRecord);
			mManifestCacheRecord = {};
		}

		mManifestAssetHandle = mnoptrAssetCache->ReloadAsset(Asset::MakeAssetFromFilePath(FilePath(), GetAssetType()));

		R2_CHECK(!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle), "The assetHandle for %s is invalid!\n", FilePath());

		mManifestCacheRecord = mnoptrAssetCache->GetAssetBuffer(mManifestAssetHandle);

		R2_CHECK(!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord), "Failed to get the asset cache record");

		mTexturePacksManifest = flat::GetTexturePacksManifest(mManifestCacheRecord.GetAssetBuffer()->Data());

		R2_CHECK(mTexturePacksManifest != nullptr, "Should never happen");
	}
#endif
}