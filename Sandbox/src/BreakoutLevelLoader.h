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

class BreakoutLevelLoader: public r2::asset::AssetLoader
{
public:
    virtual const char* GetPattern() override;
    virtual bool ShouldProcess() override;
    virtual u64 GetLoadedAssetSize(byte* rawBuffer, u64 size) override;
    virtual bool LoadAsset(byte* rawBuffer, u64 rawSize, r2::asset::AssetBuffer& assetBuffer) override;
};

#endif /* BreakoutLevelLoader_h*/
