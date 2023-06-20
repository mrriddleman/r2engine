#include "r2pch.h"
#include "r2/Core/Assets/AssetFiles/TexturePackManifestAssetFile.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Assets/AssetLib.h"

namespace r2::asset
{

	TexturePackManifestAssetFile::TexturePackManifestAssetFile()
		:mFile(nullptr)
		,mAsset({})
		,mTexturePacksManifest(nullptr)
	{
		r2::util::PathCpy(mPath, "");
	}

	TexturePackManifestAssetFile::~TexturePackManifestAssetFile()
	{
		if (mFile)
		{
			Close();
		}
	}

	bool TexturePackManifestAssetFile::Init(const char* path, r2::asset::AssetType assetType)
	{
		mAsset = r2::asset::Asset::MakeAssetFromFilePath(path, assetType);

		char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(path, sanitizedPath);

		r2::util::PathCpy(mPath, sanitizedPath);

		return mAsset.HashID() != 0;
	}

	r2::asset::AssetType TexturePackManifestAssetFile::GetAssetType() const
	{
		return r2::asset::EngineAssetType::TEXTURE_PACK_MANIFEST;
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
		return mAsset.HashID();
	}
#ifdef R2_ASSET_PIPELINE
	bool TexturePackManifestAssetFile::ReloadFilePath(const std::vector<std::string>& paths, const std::string& manifestFilePath, const byte* manifestData, r2::asset::HotReloadType type)
	{
		SetManifestData(manifestData);
		return mReloadFilePathFunc(paths, manifestFilePath, manifestData, type);
	}
#endif

	bool TexturePackManifestAssetFile::NeedsManifestData() const
	{
		return true;
	}

	void TexturePackManifestAssetFile::SetManifestData(const byte* manifestData)
	{
		mTexturePacksManifest = flat::GetTexturePacksManifest(manifestData);
	}

	bool TexturePackManifestAssetFile::Open(bool writable /*= false*/)
	{
		r2::fs::FileMode mode;
		mode = r2::fs::Mode::Read;
		mode |= r2::fs::Mode::Binary;
		if (writable)
		{
			mode |= r2::fs::Mode::Write;
		}

		return Open(mode);
	}

	bool TexturePackManifestAssetFile::Open(r2::fs::FileMode mode)
	{
		r2::fs::DeviceConfig config;
		mFile = r2::fs::FileSystem::Open(config, mPath, mode);

		return mFile != nullptr;
	}

	bool TexturePackManifestAssetFile::Close()
	{
		r2::fs::FileSystem::Close(mFile);
		mFile = nullptr;
		return true;
	}

	bool TexturePackManifestAssetFile::IsOpen() const
	{
		return mFile != nullptr;
	}

	u64 TexturePackManifestAssetFile::RawAssetSize(const Asset& asset)
	{
		return mFile->Size();
	}

	u64 TexturePackManifestAssetFile::LoadRawAsset(const Asset& asset, byte* data, u32 dataBufSize)
	{
		return mFile->ReadAll(data);
	}

	u64 TexturePackManifestAssetFile::WriteRawAsset(const Asset& asset, const byte* data, u32 dataBufferSize, u32 offset)
	{
		mFile->Seek(offset);
		return mFile->Write(data, dataBufferSize);
	}

	u64 TexturePackManifestAssetFile::NumAssets()
	{
		return 1;
	}

	void TexturePackManifestAssetFile::GetAssetName(u64 index, char* name, u32 nameBuferSize)
	{

	}

	u64 TexturePackManifestAssetFile::GetAssetHandle(u64 index)
	{
		//@TODO(Serge): Not sure yet
		return GetManifestFileHandle();
	}

	const char* TexturePackManifestAssetFile::FilePath() const
	{
		return mPath;
	}

}