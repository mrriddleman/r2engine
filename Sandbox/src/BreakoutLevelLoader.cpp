//
//  BreakoutLevelLoader.cpp
//  Sandbox
//
//  Created by Serge Lansiquot on 2019-07-03.
//

#include "BreakoutLevelLoader.h"



const char* BreakoutLevelLoader::GetPattern()
{
    return "*";
}

r2::asset::AssetType BreakoutLevelLoader::GetType() const
{
    return BREAKOUT_SCORE;
}

bool BreakoutLevelLoader::ShouldProcess()
{
    return false;
}

u64 BreakoutLevelLoader::GetLoadedAssetSize(const char* filePath, byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking)
{
    return 0;
}

bool BreakoutLevelLoader::LoadAsset(const char* filePath, byte* rawBuffer, u64 rawSize, r2::asset::AssetBuffer& assetBuffer)
{
    return false;
}
