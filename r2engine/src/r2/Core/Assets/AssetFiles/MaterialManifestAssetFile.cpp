#include "r2pch.h"

#include "r2/Core/Assets/AssetFiles/MaterialManifestAssetFile.h"
#include "r2/Core/File/File.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Render/Model/Materials/MaterialPack_generated.h"
#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/MaterialPackManifestUtils.h"

#endif


namespace r2::asset
{

	MaterialManifestAssetFile::MaterialManifestAssetFile()
		:mMaterialPackManifest(nullptr)
	{

	}

	MaterialManifestAssetFile::~MaterialManifestAssetFile()
	{

	}

	bool MaterialManifestAssetFile::Init(AssetCache* noptrAssetCache, const char* binPath, const char* rawPath, r2::asset::AssetType assetType)
	{
		mnoptrAssetCache = noptrAssetCache;
		mManifestAssetFile = (r2::asset::AssetFile*)r2::asset::lib::MakeRawAssetFile(binPath, r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(assetType));
		mAssetType = assetType;
		r2::util::PathCpy(mRawPath, rawPath);

		return mManifestAssetFile != nullptr;
	}

	void MaterialManifestAssetFile::Shutdown()
	{
		if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord) ||
			!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle))
		{
			R2_CHECK(false, "We haven't unloaded the Manifest!");
			return;
		}
	}

	r2::asset::AssetType MaterialManifestAssetFile::GetAssetType() const
	{
		return mAssetType;
	}

	bool MaterialManifestAssetFile::LoadManifest()
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

		bool success = !r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord);

		mMaterialPackManifest  = flat::GetMaterialPack(mManifestCacheRecord.GetAssetBuffer()->Data());

#ifdef R2_ASSET_PIPELINE
		
		FillMaterialVector();
#endif

		return success;
	}

	bool MaterialManifestAssetFile::UnloadManifest()
	{
		if (r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			return true;
		}

		bool success = mnoptrAssetCache->ReturnAssetBuffer(mManifestCacheRecord);
		R2_CHECK(success, "Failed to return the asset cache record");

		mManifestCacheRecord = {};
		mManifestAssetHandle = {};
		mMaterialPackManifest = nullptr;
#ifdef R2_ASSET_PIPELINE
		mMaterials.clear();
#endif
		return success;
	}

	const byte* MaterialManifestAssetFile::GetManifestData() const
	{
		if (r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			R2_CHECK(false, "Probably a bug");
			return nullptr;
		}

		return mManifestCacheRecord.GetAssetBuffer()->Data();
	}

	const char* MaterialManifestAssetFile::FilePath() const
	{
		return mManifestAssetFile->FilePath();
	}

	bool MaterialManifestAssetFile::AddAllFilePaths(FileList files)
	{
		return true;
	}

	u64 MaterialManifestAssetFile::GetManifestFileHandle() const
	{
		return r2::asset::GetAssetNameForFilePath(FilePath(), mAssetType);
	}

#ifdef R2_ASSET_PIPELINE
	bool MaterialManifestAssetFile::ReloadFilePath(const std::vector<std::string>& paths, r2::asset::HotReloadType type)
	{
		return mReloadFilePathFunc(paths, FilePath(), GetManifestData(), type);
	}

	bool MaterialManifestAssetFile::SaveManifest() 
	{	
		r2::asset::pln::SaveMaterialsToMaterialPackManifestFile(mMaterials, FilePath(), mRawPath);

		if (!AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			mnoptrAssetCache->ReturnAssetBuffer(mManifestCacheRecord);
			mManifestCacheRecord = {};
		}

		mManifestAssetHandle = mnoptrAssetCache->ReloadAsset(Asset::MakeAssetFromFilePath(FilePath(), GetAssetType()));

		R2_CHECK(!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle), "The assetHandle for %s is invalid!\n", FilePath());

		mManifestCacheRecord = mnoptrAssetCache->GetAssetBuffer(mManifestAssetHandle);

		R2_CHECK(!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord), "Failed to get the asset cache record");

		mMaterialPackManifest = flat::GetMaterialPack(mManifestCacheRecord.GetAssetBuffer()->Data());

		return mMaterialPackManifest != nullptr;
	}

	void MaterialManifestAssetFile::Reload()
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

		mMaterialPackManifest = flat::GetMaterialPack(mManifestCacheRecord.GetAssetBuffer()->Data());

#ifdef R2_ASSET_PIPELINE
		FillMaterialVector();
#endif
	}

	std::vector<r2::mat::Material>& MaterialManifestAssetFile::GetMaterials()
	{
		return mMaterials;
	}

	void MaterialManifestAssetFile::FillMaterialVector()
	{
		mMaterials.clear();
		R2_CHECK(mMaterialPackManifest != nullptr, "Should never happen");

		const auto* flatMaterials = mMaterialPackManifest->pack();
		
		mMaterials.reserve(flatMaterials->size());

		for (flatbuffers::uoffset_t i = 0; i < flatMaterials->size(); ++i)
		{
			r2::mat::Material nextMaterial;
			r2::mat::MakeMaterialFromFlatMaterial(mMaterialPackManifest->assetName(), flatMaterials->Get(i), nextMaterial);

			mMaterials.push_back(nextMaterial);
		}
	}
#endif

}