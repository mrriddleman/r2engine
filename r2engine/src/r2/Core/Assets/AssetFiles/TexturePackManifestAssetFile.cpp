#include "r2pch.h"
#include "r2/Core/Assets/AssetFiles/TexturePackManifestAssetFile.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetBuffer.h"

#ifdef R2_ASSET_PIPELINE
#include <glm/glm.hpp>
#endif

namespace r2::asset
{

	TexturePackManifestAssetFile::TexturePackManifestAssetFile()
		:mTexturePacksManifest(nullptr)
	{

	}

	TexturePackManifestAssetFile::~TexturePackManifestAssetFile()
	{
	}

	void AddAllTexturesFromTextureType(const flatbuffers::Vector<flatbuffers::Offset<flat::AssetRef>>* texturePaths, r2::asset::FileList fileList)
	{
		for (u32 i = 0; i < texturePaths->size(); ++i)
		{
			r2::asset::RawAssetFile* assetFile = r2::asset::lib::MakeRawAssetFile(texturePaths->Get(i)->binPath()->c_str(), r2::asset::TEXTURE);
			r2::sarr::Push(*fileList, (r2::asset::AssetFile*)assetFile);
		}
	}

	void AddAllTexturePathsInTexturePackToFileList(const flat::TexturePack* texturePack, r2::asset::FileList fileList)
	{
		AddAllTexturesFromTextureType(texturePack->textures(), fileList);

		if (texturePack->metaData() && texturePack->metaData()->mipLevels())
		{
			for (flatbuffers::uoffset_t i = 0; i < texturePack->metaData()->mipLevels()->size(); ++i)
			{
				const flat::MipLevel* mipLevel = texturePack->metaData()->mipLevels()->Get(i);

				for (flatbuffers::uoffset_t side = 0; side < mipLevel->sides()->size(); ++side)
				{
					r2::asset::RawAssetFile* assetFile = r2::asset::lib::MakeRawAssetFile(mipLevel->sides()->Get(side)->textureName()->c_str(), r2::asset::CUBEMAP_TEXTURE);
					r2::sarr::Push(*fileList, (r2::asset::AssetFile*)assetFile);
				}
			}
		}
	}

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

		FillAssetFiles();

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
		R2_CHECK(mTexturePacksManifest != nullptr, "Should never happen");
		R2_CHECK(asset.GetType() == r2::asset::TEXTURE || asset.GetType() == r2::asset::CUBEMAP_TEXTURE || asset.GetType() == TEXTURE_PACK, "These are the only asset types supported");

		const auto* flatTexturePacks = mTexturePacksManifest->texturePacks();

		if (asset.GetType() == TEXTURE_PACK)
		{
			for (flatbuffers::uoffset_t i = 0; i < flatTexturePacks->size(); ++i)
			{
				//@TODO(Serge): UUID
				if (flatTexturePacks->Get(i)->assetName()->assetName() == asset.HashID())
				{
					return true;
				}
			}
		}
		else
		{
			for (flatbuffers::uoffset_t i = 0; i < flatTexturePacks->size(); ++i)
			{
				const auto* texturePack = flatTexturePacks->Get(i);

				const auto textures = texturePack->textures();

				if (texturePack->metaData()->type() == flat::TextureType_TEXTURE && asset.GetType() == TEXTURE ||
					texturePack->metaData()->type() == flat::TextureType_CUBEMAP && asset.GetType() == CUBEMAP_TEXTURE)
				{
					for (flatbuffers::uoffset_t j = 0; j < textures->size(); ++j)
					{
						//@TODO(Serge): UUID
						if (textures->Get(j)->assetName()->assetName() == asset.HashID())
						{
							return true;
						}
					}
				}
			}
		}

		return false;
	}

	AssetFile* TexturePackManifestAssetFile::GetAssetFile(const Asset& asset)
	{
		R2_CHECK(mTexturePacksManifest != nullptr, "Should never happen");
		R2_CHECK(asset.GetType() != r2::asset::TEXTURE_PACK, "We need to figure out a way to represent a TEXTURE_PACK as a file - if we care");
		R2_CHECK(asset.GetType() == r2::asset::TEXTURE || asset.GetType() == r2::asset::CUBEMAP_TEXTURE, "These are the only asset types supported");

		for (u32 i = 0; i < r2::sarr::Size(*mAssetFiles); ++i)
		{
			AssetFile* assetFile = r2::sarr::At(*mAssetFiles, i);
			//@TODO(Serge): UUID
			if (assetFile->GetAssetHandle(0) == asset.HashID())
			{
				return assetFile;
			}
		}

		return nullptr;
	}

	void TexturePackManifestAssetFile::DestroyAssetFiles()
	{
		s32 numFiles = r2::sarr::Size(*mAssetFiles);

		for (s32 i = numFiles - 1; i >= 0; --i)
		{
			RawAssetFile* rawAssetFile = (RawAssetFile*)(r2::sarr::At(*mAssetFiles, i));

			lib::FreeRawAssetFile(rawAssetFile);
		}

		lib::DestoryFileList(mAssetFiles);

		mAssetFiles = nullptr;
	}

	void TexturePackManifestAssetFile::FillAssetFiles()
	{
		u64 numTextureFiles = mTexturePacksManifest->totalNumberOfTextures();

#ifdef R2_ASSET_PIPELINE
		numTextureFiles = glm::max(2000llu, numTextureFiles);
#endif

		mAssetFiles = lib::MakeFileList(numTextureFiles);

		auto texturePacks = mTexturePacksManifest->texturePacks();

		for (flatbuffers::uoffset_t i = 0; i < texturePacks->size(); ++i)
		{
			const flat::TexturePack* texturePack = texturePacks->Get(i);

			AddAllTexturePathsInTexturePackToFileList(texturePack, mAssetFiles);
		}
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

		FillAssetFiles();
	}
#endif
}