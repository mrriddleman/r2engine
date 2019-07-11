//
//  BreakoutLevelsFile.cpp
//  Sandbox
//
//  Created by Serge Lansiquot on 2019-07-03.
//

#include "BreakoutLevelsFile.h"
#include "r2/Core/File/FileSystem.h"
#include "flatbuffers/flatbuffers.h"

BreakoutLevelsFile::BreakoutLevelsFile(): mFile(nullptr)
{
    
    strcpy(mPath, "");
}

bool BreakoutLevelsFile::Init(const char* path)
{
    if (!path)
    {
        return false;
    }

    strcpy(mPath, path);
    return strcmp(mPath, "") != 0;
}

bool BreakoutLevelsFile::Open()
{
    r2::fs::DeviceConfig config;
    r2::fs::FileMode mode;
    mode = r2::fs::Mode::Read;
    mode |= r2::fs::Mode::Binary;
    
    mFile = r2::fs::FileSystem::Open(config, mPath, mode);
    
    return mFile != nullptr;
}

bool BreakoutLevelsFile::Close()
{
    r2::fs::FileSystem::Close(mFile);
    return true;
}

bool BreakoutLevelsFile::IsOpen()
{
    return mFile != nullptr;
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
