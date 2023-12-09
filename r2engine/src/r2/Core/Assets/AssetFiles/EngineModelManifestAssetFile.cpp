#include "r2pch.h"

#include "r2/Core/Assets/AssetFiles/EngineModelManifestAssetFile.h"
#include "r2/Render/Model/ModelManifest_generated.h"
#include "r2/Core/Assets/AssetBuffer.h"

namespace r2::asset
{
	EngineModelManifestAssetFile::EngineModelManifestAssetFile()
		:mModelsManifest(nullptr)
	{

	}
	EngineModelManifestAssetFile::~EngineModelManifestAssetFile()
	{

	}

	bool EngineModelManifestAssetFile::LoadManifest()
	{
		bool success = ManifestAssetFile::LoadManifest();

		mModelsManifest = flat::GetModelsManifest(mManifestCacheRecord.GetAssetBuffer()->Data());

		return success;
	}

	bool EngineModelManifestAssetFile::UnloadManifest()
	{
		bool success = ManifestAssetFile::UnloadManifest();

		mModelsManifest = nullptr;

		return success;
	}

	bool EngineModelManifestAssetFile::HasAsset(const Asset& asset) const
	{
		if (asset.GetType() == r2::asset::MESH)
		{
			for (flatbuffers::uoffset_t i = 0; i < mModelsManifest->meshes()->size(); ++i)
			{
				//@TODO(Serge): should be UUID
				if (mModelsManifest->meshes()->Get(i)->assetName()->assetName() == asset.HashID())
				{
					return true;
				}
			}
		}
		else if (asset.GetType() == r2::asset::MODEL)
		{
			for (flatbuffers::uoffset_t i = 0; i < mModelsManifest->models()->size(); ++i)
			{
				//@TODO(Serge): should be UUID
				if (mModelsManifest->models()->Get(i)->assetName()->assetName() == asset.HashID())
				{
					return true;
				}
			}
		}

		return false;
	}

#ifdef R2_ASSET_PIPELINE
	bool EngineModelManifestAssetFile::AddAssetReference(const AssetReference& assetReference)
	{
		//Never happens
		return false;
	}

	bool EngineModelManifestAssetFile::SaveManifest()
	{
		return ManifestAssetFile::SaveManifest();
	}

	void EngineModelManifestAssetFile::Reload()
	{
		ManifestAssetFile::Reload();
	}
#endif

}