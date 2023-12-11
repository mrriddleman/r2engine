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
#include <filesystem>
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

	bool MaterialManifestAssetFile::LoadManifest()
	{
		bool success = ManifestAssetFile::LoadManifest();

		mMaterialPackManifest  = flat::GetMaterialPack(mManifestCacheRecord.GetAssetBuffer()->Data());

#ifdef R2_ASSET_PIPELINE
		
		FillMaterialVector();
#endif

		return success && !mMaterialPackManifest;
	}

	bool MaterialManifestAssetFile::UnloadManifest()
	{
		bool success = ManifestAssetFile::UnloadManifest();

		mMaterialPackManifest = nullptr;
#ifdef R2_ASSET_PIPELINE
		mMaterials.clear();
#endif
		return success;
	}

	bool MaterialManifestAssetFile::HasAsset(const Asset& asset) const
	{
		return false;
	}

	AssetFile* MaterialManifestAssetFile::GetAssetFile(const Asset& asset)
	{
		TODO;
		return nullptr;
	}

	void MaterialManifestAssetFile::DestroyAssetFiles()
	{
		mAssetFiles = nullptr;
	}


#ifdef R2_ASSET_PIPELINE

	void MaterialManifestAssetFile::ReloadManifestFile(bool fillVector)
	{
		ManifestAssetFile::Reload();

		mMaterialPackManifest = flat::GetMaterialPack(mManifestCacheRecord.GetAssetBuffer()->Data());

		if (fillVector)
		{
			FillMaterialVector();
		}
	}

	bool MaterialManifestAssetFile::SaveManifest() 
	{	
		r2::asset::pln::SaveMaterialsToMaterialPackManifestFile(mMaterials, FilePath(), mRawPath);

		ReloadManifestFile(false);

		return mMaterialPackManifest != nullptr;
	}

	void MaterialManifestAssetFile::Reload()
	{
		ReloadManifestFile(true);
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
			r2::mat::MakeMaterialFromFlatMaterial(mMaterialPackManifest->assetName()->assetName(), flatMaterials->Get(i), nextMaterial);

			mMaterials.push_back(nextMaterial);
		}
	}

	bool MaterialManifestAssetFile::AddAssetReference(const AssetReference& assetReference)
	{
		TODO;
		return false;
	}

#endif

}