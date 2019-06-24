//
//  AssetBuffer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-23.
//

#include "AssetBuffer.h"

namespace r2::asset
{
    AssetBuffer::AssetBuffer()
        : moptrData(nullptr)
        , mDataSize(0)
    {
        
    }
    
    void AssetBuffer::Load(const Asset& asset, char* data, u64 dataSize)
    {
        mAsset = asset;
        moptrData = data;
        mDataSize = dataSize;
    }
}
