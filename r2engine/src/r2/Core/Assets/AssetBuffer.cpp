//
//  AssetBuffer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-23.
//
#include "r2pch.h"
#include "AssetBuffer.h"

namespace r2::asset
{
    AssetBuffer::AssetBuffer()
        : moptrData(nullptr)
        , mDataSize(0)
    {
    }
    
    void AssetBuffer::Load( byte* data, u64 dataSize)
    {
        moptrData = data;
        mDataSize = dataSize;
    }
}
