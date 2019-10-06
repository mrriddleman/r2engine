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

namespace r2::asset::pln::audio
{
    bool GenerateSoundDefinitionsFromJson(const std::string& soundDefinitionFilePath);
    bool GenerateSoundDefinitionsFromDirectories(const std::string& filePath, const std::vector<std::string>& directories);
    
    bool FindSoundDefinitionFile(const std::string& directory, std::string& outPath, bool isBinary);
    
    std::vector<r2::audio::AudioEngine::SoundDefinition> LoadSoundDefinitions(const std::string& soundDefinitionFilePath);
    
    bool GenerateSoundDefinitionsFile(const std::string& path, const std::vector<r2::audio::AudioEngine::SoundDefinition>& soundDefinitions);
    
    bool AddNewSoundDefinition(std::vector<r2::audio::AudioEngine::SoundDefinition>& soundDefinitions, const r2::audio::AudioEngine::SoundDefinition& def);
}
#endif
#endif /* SoundDefinitionUtils_h */