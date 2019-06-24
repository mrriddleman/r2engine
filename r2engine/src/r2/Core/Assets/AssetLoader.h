//
//  AssetLoader.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-23.
//

#ifndef AssetLoader_h
#define AssetLoader_h

namespace r2::asset
{
    class AssetBuffer;
    
    class AssetLoader
    {
    public:
        
        virtual bool ShouldProcess() = 0;
        virtual u64 GetLoadedAssetSize(byte* rawBuffer, u64 size) = 0;
        virtual bool LoadAsset(byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer) = 0;
    };
}

#endif /* AssetLoader_h */
