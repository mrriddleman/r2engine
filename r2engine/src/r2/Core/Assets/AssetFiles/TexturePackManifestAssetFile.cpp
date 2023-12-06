#include "r2pch.h"
#include "r2/Core/Assets/AssetFiles/TexturePackManifestAssetFile.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"

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

	//void AddAllTexturesFromTextureType(const flatbuffers::Vector<flatbuffers::Offset<flat::AssetRef>>* texturePaths, r2::asset::FileList fileList)
	//{
	//	for (u32 i = 0; i < texturePaths->size(); ++i)
	//	{
	//		r2::asset::RawAssetFile* assetFile = r2::asset::lib::MakeRawAssetFile(texturePaths->Get(i)->binPath()->c_str(), r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::TEXTURE));
	//		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)assetFile);
	//	}
	//}

	//void AddAllTexturePathsInTexturePackToFileList(const flat::TexturePack* texturePack, r2::asset::FileList fileList)
	//{
	//	AddAllTexturesFromTextureType(texturePack->textures(), fileList);

	//	if (texturePack->metaData() && texturePack->metaData()->mipLevels())
	//	{
	//		for (flatbuffers::uoffset_t i = 0; i < texturePack->metaData()->mipLevels()->size(); ++i)
	//		{
	//			const flat::MipLevel* mipLevel = texturePack->metaData()->mipLevels()->Get(i);

	//			for (flatbuffers::uoffset_t side = 0; side < mipLevel->sides()->size(); ++side)
	//			{
	//				r2::asset::RawAssetFile* assetFile = r2::asset::lib::MakeRawAssetFile(mipLevel->sides()->Get(side)->textureName()->c_str(), r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::CUBEMAP_TEXTURE));
	//				r2::sarr::Push(*fileList, (r2::asset::AssetFile*)assetFile);
	//			}
	//		}
	//	}
	//}

	//bool TexturePackManifestAssetFile::AddAllFilePaths(FileList fileList)
	//{
	//	auto texturePacks = mTexturePacksManifest->texturePacks();

	//	for (flatbuffers::uoffset_t i = 0; i < texturePacks->size(); ++i)
	//	{
	//		const flat::TexturePack* texturePack = texturePacks->Get(i);

	//		AddAllTexturePathsInTexturePackToFileList(texturePack, fileList);
	//	}

	//	return true;
	//}

	bool TexturePackManifestAssetFile::LoadManifest()
	{
		bool success = ManifestAssetFile::LoadManifest();
		if (!success)
		{
			R2_CHECK(false, "Failed to load the manifest file: %s\n", FilePath());
			return false;
		}

		mTexturePacksManifest = flat::GetTexturePacksManifest(mManifestCacheRecord.GetAssetBuffer()->Data());

		R2_CHECK(mTexturePacksManifest != nullptr, "Should never happen");

		return success && !mTexturePacksManifest;
	}

	bool TexturePackManifestAssetFile::UnloadManifest()
	{
		bool success = ManifestAssetFile::UnloadManifest();

		mTexturePacksManifest = nullptr;

		return success;
	}

	bool TexturePackManifestAssetFile::HasAsset(const Asset& asset) const
	{
		return false;
	}

#ifdef R2_ASSET_PIPELINE

	bool TexturePackManifestAssetFile::AddAssetReference(const AssetReference& assetReference)
	{
		TODO;
		return false;
	}

	bool TexturePackManifestAssetFile::SaveManifest()
	{
		TODO;
		return false;
	}

	void TexturePackManifestAssetFile::Reload()
	{
		ManifestAssetFile::Reload();

		mTexturePacksManifest = flat::GetTexturePacksManifest(mManifestCacheRecord.GetAssetBuffer()->Data());

		R2_CHECK(mTexturePacksManifest != nullptr, "Should never happen");
	}
#endif
}