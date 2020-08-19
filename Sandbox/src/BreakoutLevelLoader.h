//
//  BreakoutLevelLoader.h
//  Sandbox
//
//  Created by Serge Lansiquot on 2019-07-03.
//

#ifndef BreakoutLevelLoader_h
#define BreakoutLevelLoader_h

#include "r2.h"
#include "r2/Core/Assets/AssetLoader.h"

namespace r2::asset
{
    class AssetBuffer;
}

enum BreakoutAssetType: u32
{
    //@TODO(Serge): figure out a better way to do this
    BREAKOUT_SCORE = r2::asset::EngineAssetType::NUM_ENGINE_ASSET_TYPES + 1
};

class BreakoutLevelLoader: public r2::asset::AssetLoader
{
public:
    virtual const char* GetPattern() override;
    virtual r2::asset::AssetType GetType() const override;
    virtual bool ShouldProcess() override;
    virtual u64 GetLoadedAssetSize(byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking) override;
    virtual bool LoadAsset(byte* rawBuffer, u64 rawSize, r2::asset::AssetBuffer& assetBuffer) override;
};

#endif /* BreakoutLevelLoader_h*/
