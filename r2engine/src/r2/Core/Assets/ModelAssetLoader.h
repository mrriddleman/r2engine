#ifndef __MODEL_ASSET_LOADER_H__
#define __MODEL_ASSET_LOADER_H__

#include "r2/Core/Assets/AssetLoader.h"

namespace r2::draw
{
	struct ModelSystem;
}


namespace r2::asset
{
	class ModelAssetLoader : public AssetLoader
	{
	public:

		void SetModelSystem(r2::draw::ModelSystem* modelSystem) { mnoptrModelSystem = modelSystem; }
		//@TODO(Serge): we should pass some kind of lookup table that will map the name of the mesh to data of the mesh
		virtual const char* GetPattern() override;
		virtual AssetType GetType() const override;
		virtual bool ShouldProcess() override;
		virtual u64 GetLoadedAssetSize(const char* filePath, byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking) override;
		virtual bool LoadAsset(const char* filePath, byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer) override;

	private:
		r2::draw::ModelSystem* mnoptrModelSystem = nullptr;
	};
}


#endif