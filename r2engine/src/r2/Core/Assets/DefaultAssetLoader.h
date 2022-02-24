//
//  DefaultAssetLoader.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-07-02.
//

#ifndef DefaultAssetLoader_h
#define DefaultAssetLoader_h

#include "r2/Core/Assets/AssetLoader.h"

namespace r2::asset
{
    class DefaultAssetLoader: public AssetLoader
    {
    public:
        virtual const char* GetPattern() override;
        virtual AssetType GetType() const override;
        virtual bool ShouldProcess() override;
        virtual u64 GetLoadedAssetSize(const char* filePath, byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking) override;
        virtual bool LoadAsset(const char* filePath, byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer) override;
    };
}

#endif /* DefaultAssetLoader_h */
