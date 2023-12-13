#include "r2pch.h"

#include "r2/Core/Assets/AssetFiles/EngineModelManifestAssetFile.h"
#include "r2/Render/Model/ModelManifest_generated.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetLib.h"
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

		FillAssetFiles();

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

	AssetFile* EngineModelManifestAssetFile::GetAssetFile(const Asset& asset)
	{
		R2_CHECK(asset.GetType() == MESH || asset.GetType() == MODEL, "MESH and MODEL are the only types that are supported");

		u32 offset = 0;
		u32 size = mModelsManifest->models()->size();

		if (asset.GetType() == r2::asset::MESH)
		{
			offset = mModelsManifest->models()->size();
			size += mModelsManifest->meshes()->size();
		}

		for (u32 i = offset; i < size; ++i)
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

	void EngineModelManifestAssetFile::FillAssetFiles()
	{
		R2_CHECK(mModelsManifest != nullptr, "The mModelsManifest is nullptr?");

		u32 numAssets = mModelsManifest->models()->size() + mModelsManifest->meshes()->size();

		mAssetFiles = lib::MakeFileList(numAssets);

		R2_CHECK(mAssetFiles != nullptr, "We couldn't create the File List?");

		for (u32 i = 0; i < mModelsManifest->models()->size(); ++i)
		{
			r2::sarr::Push(*mAssetFiles, (r2::asset::AssetFile*)lib::MakeRawAssetFile(mModelsManifest->models()->Get(i)->binPath()->str().c_str(), r2::asset::MODEL));
		}

		for (u32 i = 0; i < mModelsManifest->meshes()->size(); ++i)
		{
			r2::sarr::Push(*mAssetFiles, (r2::asset::AssetFile*)lib::MakeRawAssetFile(mModelsManifest->meshes()->Get(i)->binPath()->str().c_str(), r2::asset::MESH));
		}
	}

	void EngineModelManifestAssetFile::DestroyAssetFiles()
	{
		s32 numFiles = r2::sarr::Size(*mAssetFiles);

		for (s32 i = numFiles - 1; i >= 0; --i)
		{
			RawAssetFile* rawAssetFile = (RawAssetFile*)( r2::sarr::At(*mAssetFiles, i) );

			lib::FreeRawAssetFile(rawAssetFile);
		}

		lib::DestoryFileList(mAssetFiles);

		mAssetFiles = nullptr;
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

		FillAssetFiles();
	}

#endif

}