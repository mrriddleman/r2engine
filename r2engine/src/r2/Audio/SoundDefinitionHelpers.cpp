#include "r2pch.h"

#include "r2/Audio/SoundDefinitionHelpers.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Audio/SoundDefinition_generated.h"

#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"

namespace r2::audio
{
	const char* GetSoundBankNameFromAssetName(u64 soundBankAssetName)
	{
		if (soundBankAssetName == 0)
		{
			return "";
		}

		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();
		u32 count = r2::asset::lib::GetManifestDataCountForType(assetLib, asset::SOUND_DEFINTION);

		r2::SArray<const byte*>* soundDefinitions = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const byte*, count);

		r2::asset::lib::GetManifestDataForType(assetLib, asset::SOUND_DEFINTION, soundDefinitions);
		
		for (u32 i = 0; i < count; ++i)
		{
			const byte* soundDefinitionData = r2::sarr::At(*soundDefinitions, i);

			const flat::SoundDefinitions* flatSoundDef = flat::GetSoundDefinitions(soundDefinitionData);

			for (flatbuffers::uoffset_t j = 0; j < flatSoundDef->banks()->size(); ++j)
			{
				const char* soundBankName = flatSoundDef->banks()->Get(j)->path()->c_str();
				u64 flatSoundBankAssetName = r2::asset::GetAssetNameForFilePath(soundBankName, r2::asset::SOUND);

				if (flatSoundBankAssetName == soundBankAssetName)
				{
					FREE(soundDefinitions, *MEM_ENG_SCRATCH_PTR);
					return soundBankName;
				}
			}
		}

		FREE(soundDefinitions, *MEM_ENG_SCRATCH_PTR);
		return "";
	}
}