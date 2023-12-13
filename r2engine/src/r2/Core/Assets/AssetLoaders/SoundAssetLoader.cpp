#include "r2pch.h"

#include "r2/Core/Assets/AssetLoaders/SoundAssetLoader.h"
#include "r2/Audio/AudioEngine.h"
#include "r2/Core/Assets/AssetBuffer.h"

namespace r2::asset
{

	const char* SoundAssetLoader::GetPattern()
	{
		return ".bank";
	}

	r2::asset::AssetType SoundAssetLoader::GetType() const
	{
		return r2::asset::SOUND;
	}

	bool SoundAssetLoader::ShouldProcess()
	{
		return true;
	}

	u64 SoundAssetLoader::GetLoadedAssetSize(const char* filePath, byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking)
	{
		return size;
	}

	bool SoundAssetLoader::LoadAsset(const char* filePath, byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer)
	{
		auto bankHandle = r2::audio::AudioEngine::LoadBank(filePath, r2::audio::AudioEngine::LOAD_BANK_NORMAL);
		memcpy(assetBuffer.MutableData(), &bankHandle, rawSize);
		return true;
	}

	bool SoundAssetLoader::FreeAsset(const AssetBuffer& assetBuffer)
	{
		const byte* bankHandleData = assetBuffer.Data();

		const r2::audio::AudioEngine::BankHandle* bankHandlePtr = (const r2::audio::AudioEngine::BankHandle*)bankHandleData;

		r2::audio::AudioEngine::UnloadSoundBank(*bankHandlePtr);

		return true;
	}

}