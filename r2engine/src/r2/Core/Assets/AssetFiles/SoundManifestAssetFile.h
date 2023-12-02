#ifndef __SOUND_MANIFEST_ASSET_FILE_H__
#define __SOUND_MANIFEST_ASSET_FILE_H__

#include "r2/Core/Assets/AssetFiles/ManifestAssetFile.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/AssetReference.h"
#endif
namespace flat
{
	struct SoundDefinitions;
}

namespace r2::asset 
{
	class SoundManifestAssetFile : public ManifestAssetFile
	{
	public:

		virtual bool LoadManifest() override;
		virtual bool UnloadManifest() override;
		virtual bool HasAsset(const Asset& asset) const override;

		bool AddAllFilePaths(FileList files) override;

#ifdef R2_ASSET_PIPELINE
		virtual bool AddAssetReference(const AssetReference& assetReference)override;
		virtual bool SaveManifest()override;
		virtual void Reload()override;
#endif

	private:
		const flat::SoundDefinitions* mSoundDefinitions;

#ifdef R2_ASSET_PIPELINE
		void FillSoundDefinitionVector();
		void ReloadManifestFile(bool fillVector);

		std::vector<r2::asset::AssetReference> mSoundDefinitionVector;
#endif

	};
}

#endif