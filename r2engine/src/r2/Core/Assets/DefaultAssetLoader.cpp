//
//  DefaultAssetLoader.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-07-02.
//
#include "r2pch.h"
#include "DefaultAssetLoader.h"

namespace r2::asset
{
    const char* DefaultAssetLoader::GetPattern()
    {
        return "*";
    }

    AssetType DefaultAssetLoader::GetType() const
    {
        return DEFAULT;
    }
    
    bool DefaultAssetLoader::ShouldProcess()
    {
        return false;
    }
    
    u64 DefaultAssetLoader::GetLoadedAssetSize(const char* filePath, byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking)
    {
        return size;
    }
    
    bool DefaultAssetLoader::LoadAsset(const char* filePath, byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer)
    {
        return true;
    }
}
