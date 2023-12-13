//
//  AssetLoader.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-23.
//

#ifndef AssetLoader_h
#define AssetLoader_h

#include "r2/Core/Assets/AssetTypes.h"


namespace r2::asset
{
    class AssetBuffer;
    
    class AssetLoader
    {
    public:
        
        virtual const char* GetPattern() = 0;
        virtual bool ShouldProcess() = 0;
        virtual AssetType GetType() const = 0;
        virtual u64 GetLoadedAssetSize(const char* filePath, byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking) = 0;
        virtual bool LoadAsset(const char* filePath, byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer) = 0;
        virtual bool FreeAsset(const AssetBuffer& assetBuffer) = 0;

        //virtual bool 
        virtual ~AssetLoader(){}
    };
}

#endif /* AssetLoader_h */
