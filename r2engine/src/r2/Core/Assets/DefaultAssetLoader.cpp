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
    
    bool DefaultAssetLoader::ShouldProcess()
    {
        return false;
    }
    
    u64 DefaultAssetLoader::GetLoadedAssetSize(byte* rawBuffer, u64 size)
    {
        return size;
    }
    
    bool DefaultAssetLoader::LoadAsset(byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer)
    {
        return true;
    }
}
