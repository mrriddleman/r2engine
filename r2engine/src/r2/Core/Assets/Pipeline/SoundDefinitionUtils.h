//
//  SoundDefinitionUtils.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-09-25.
//

#ifndef SoundDefinitionUtils_h
#define SoundDefinitionUtils_h
#ifdef R2_ASSET_PIPELINE
#include "r2/Audio/AudioEngine.h"
#include "r2/Core/Assets/AssetReference.h"

namespace r2::asset::pln::audio
{
    bool GenerateSoundDefinitionsFromJson(const std::string& soundDefinitionFilePath);
    bool GenerateSoundDefinitionsFromDirectories(u32 version, const std::string& binFilePath, const std::string& jsonFilePath, const std::vector<std::string>& directories);
    
    bool SaveSoundDefinitionsManifestAssetReferencesToManifestFile(u32 version, const std::vector<r2::asset::AssetReference>& assetReferences, const std::string& binFilePath, const std::string& rawFilePath);
    bool FindSoundDefinitionFile(const std::string& directory, std::string& outPath, bool isBinary);
    bool GenerateSoundDefinitionsFile(u32 version, const std::string& binFilePath, const std::string& jsonFilePath, const r2::asset::AssetReference& masterBank, const r2::asset::AssetReference& masterBankStrings, const std::vector<r2::asset::AssetReference>& bankFiles);
}
#endif
#endif /* SoundDefinitionUtils_h */
