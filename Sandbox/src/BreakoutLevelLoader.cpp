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

bool BreakoutLevelLoader::ShouldProcess()
{
    return true;
}

u64 BreakoutLevelLoader::GetLoadedAssetSize(byte* rawBuffer, u64 size)
{
    return 0;
}

bool BreakoutLevelLoader::LoadAsset(byte* rawBuffer, u64 rawSize, r2::asset::AssetBuffer& assetBuffer)
{
    return false;
}
