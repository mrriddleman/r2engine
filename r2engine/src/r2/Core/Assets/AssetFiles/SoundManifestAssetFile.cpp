#include "r2pch.h"

#include "r2/Core/Assets/AssetFiles/SoundManifestAssetFile.h"
#include "r2/Audio/SoundDefinition_generated.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/Pipeline/SoundDefinitionUtils.h"

namespace r2::asset 
{

	bool SoundManifestAssetFile::LoadManifest()
	{
		bool success = ManifestAssetFile::LoadManifest();

		R2_CHECK(success, "Couldn't load the Manifest for the sound definitions");
		if (!success)
		{
			return false;
		}

		mSoundDefinitions = flat::GetSoundDefinitions(mManifestCacheRecord.GetAssetBuffer()->Data());

		R2_CHECK(mSoundDefinitions != nullptr, "mSoundDefinitions is nullptr?");

#ifdef R2_ASSET_PIPELINE
		FillSoundDefinitionVector();
#endif

		return success && mSoundDefinitions != nullptr;
	}

	bool SoundManifestAssetFile::UnloadManifest()
	{
		bool success = ManifestAssetFile::UnloadManifest();

		mSoundDefinitions = nullptr;
#ifdef R2_ASSET_PIPELINE
		mSoundDefinitionVector.clear();
#endif

		return success;
	}

	bool SoundManifestAssetFile::HasAsset(const Asset& asset) const
	{
		//@TODO(Serge): UUID
		if (mSoundDefinitions->masterBank()->assetName()->assetName() == asset.HashID())
		{
			return true;
		}

		//@TODO(Serge): UUID
		if (mSoundDefinitions->masterBankStrings()->assetName()->assetName() == asset.HashID());
		{
			return true;
		}

		//@TODO(Serge): UUID
		for (flatbuffers::uoffset_t i = 0; i < mSoundDefinitions->banks()->size(); ++i)
		{
			if (mSoundDefinitions->banks()->Get(i)->assetName()->assetName() == asset.HashID())
			{
				return true;
			}
		}

		return false;
	}

	//bool SoundManifestAssetFile::AddAllFilePaths(FileList files) 
	//{
	//	return true;
	//}

#ifdef R2_ASSET_PIPELINE
	bool SoundManifestAssetFile::AddAssetReference(const AssetReference& assetReference)
	{
		//first check to see if we have it already
		bool hasAssetReference = false;
		//if we don't add it
		for (u32 i = 0; i < mSoundDefinitionVector.size(); ++i)
		{
			if (mSoundDefinitionVector[i].assetName == assetReference.assetName)
			{
				hasAssetReference = true;
				break;
			}
		}

		if (!hasAssetReference)
		{
			r2::asset::AssetReference newSoundDefinition = assetReference;
			//@TODO(Serge): maybe move this to the EditorAssetPanel?
			std::filesystem::path filePathURI = newSoundDefinition.binPath.lexically_relative(std::filesystem::path(mRawPath).parent_path());

			newSoundDefinition.binPath = filePathURI;
			newSoundDefinition.rawPath = filePathURI;

			mSoundDefinitionVector.push_back(newSoundDefinition);

			//then regen the manifest
			return SaveManifest();
		}

		return hasAssetReference;
	}

	bool SoundManifestAssetFile::SaveManifest()
	{
		r2::asset::pln::audio::SaveSoundDefinitionsManifestAssetReferencesToManifestFile(1, mSoundDefinitionVector, FilePath(), mRawPath);

		ReloadManifestFile(false);

		return mSoundDefinitions != nullptr;
	}

	void SoundManifestAssetFile::Reload()
	{
		ReloadManifestFile(true);
	}

	void SoundManifestAssetFile::ReloadManifestFile(bool fillVector)
	{
		ManifestAssetFile::Reload();

		mSoundDefinitions = flat::GetSoundDefinitions(mManifestCacheRecord.GetAssetBuffer()->Data());

		R2_CHECK(mSoundDefinitions != nullptr, "mSoundDefinitions is nullptr?");

		if (fillVector)
		{
			FillSoundDefinitionVector();
		}
	}

	void SoundManifestAssetFile::FillSoundDefinitionVector()
	{
		mSoundDefinitionVector.clear();
		R2_CHECK(mSoundDefinitions != nullptr, "Should never happen");

		const auto* flatBanks = mSoundDefinitions->banks();

		mSoundDefinitionVector.reserve(flatBanks->size() + 2);

		r2::asset::AssetReference masterBankAssetReference;
		r2::asset::MakeAssetReferenceFromFlatAssetRef(mSoundDefinitions->masterBank(), masterBankAssetReference);

		mSoundDefinitionVector.push_back(masterBankAssetReference);

		r2::asset::AssetReference masterBankStringAssetReference;
		r2::asset::MakeAssetReferenceFromFlatAssetRef(mSoundDefinitions->masterBankStrings(), masterBankStringAssetReference);
		mSoundDefinitionVector.push_back(masterBankStringAssetReference);

		for (flatbuffers::uoffset_t i = 0; i < flatBanks->size(); ++i)
		{
			r2::asset::AssetReference bankAssetReference;
			r2::asset::MakeAssetReferenceFromFlatAssetRef(mSoundDefinitions->banks()->Get(i), bankAssetReference);

			mSoundDefinitionVector.push_back(bankAssetReference);
		}
	}
#endif

}