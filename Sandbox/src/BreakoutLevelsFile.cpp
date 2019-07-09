//
//  BreakoutLevelsFile.cpp
//  Sandbox
//
//  Created by Serge Lansiquot on 2019-07-03.
//

#include "BreakoutLevelsFile.h"

bool BreakoutLevelsFile::Open()
{
    return false;
}

bool BreakoutLevelsFile::Close()
{
    return false;
}

bool BreakoutLevelsFile::IsOpen()
{
    return false;
}

u64 BreakoutLevelsFile::RawAssetSize(const r2::asset::Asset& asset)
{
    return 0;
}

u64 BreakoutLevelsFile::RawAssetSizeFromHandle(u64 handle)
{
    return 0;
}

u64 BreakoutLevelsFile::GetRawAsset(const r2::asset::Asset& asset, byte* data)
{
    return 0;
}

u64 BreakoutLevelsFile::GetRawAssetFromHandle(u64 handle, byte* data)
{
    return 0;
}

u64 BreakoutLevelsFile::NumAssets() const
{
    return 0;
}

char* BreakoutLevelsFile::GetAssetName(u64 index) const
{
    return 0;
}

u64 BreakoutLevelsFile::GetAssetHandle(u64 index) const
{
    return 0;
}
