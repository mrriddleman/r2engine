//
//  BreakoutLevelsFile.cpp
//  Sandbox
//
//  Created by Serge Lansiquot on 2019-07-03.
//

#include "BreakoutLevelsFile.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"

BreakoutLevelsFile::BreakoutLevelsFile(): mFile(nullptr)
{
    r2::util::PathCpy(mPath, "");
}

BreakoutLevelsFile::~BreakoutLevelsFile()
{
    if (mFile)
    {
        Close();
    }
}

bool BreakoutLevelsFile::Init(const char* path)
{
    if (!path)
    {
        return false;
    }

    r2::util::PathCpy(mPath, path);
    return strcmp(mPath, "") != 0;
}

bool BreakoutLevelsFile::Open(bool writable)
{
    r2::fs::FileMode mode;
    mode = r2::fs::Mode::Read;
    mode |= r2::fs::Mode::Binary;
    if (writable)
    {
        mode |= r2::fs::Mode::Write;
    }

    return Open(mode);
}

bool BreakoutLevelsFile::Open(r2::fs::FileMode mode)
{
    r2::fs::DeviceConfig config;
	mFile = r2::fs::FileSystem::Open(config, mPath, mode);
	return mFile != nullptr;
}

bool BreakoutLevelsFile::Close()
{
    r2::fs::FileSystem::Close(mFile);
    mFile = nullptr;
    return true;
}

bool BreakoutLevelsFile::IsOpen() const
{
    return mFile != nullptr;
}

u64 BreakoutLevelsFile::RawAssetSize(const r2::asset::Asset& asset)
{
    return mFile->Size();
}

u64 BreakoutLevelsFile::LoadRawAsset(const r2::asset::Asset& asset, byte* data, u32 dataBufSize)
{
    return mFile->ReadAll(data);
}

u64 BreakoutLevelsFile::WriteRawAsset(const r2::asset::Asset& asset, const byte* data, u32 dataBufferSize, u32 offset)
{
    return 0;
}

u64 BreakoutLevelsFile::NumAssets()
{
    return 1;
}

void BreakoutLevelsFile::GetAssetName(u64 index, char* name, u32 nameBufSize)
{
    r2::fs::utils::CopyFileNameWithExtension(mPath, name);
}

u64 BreakoutLevelsFile::GetAssetHandle(u64 index)
{
    char fileName[r2::fs::FILE_PATH_LENGTH];
    r2::fs::utils::CopyFileNameWithExtension(mPath, fileName);
    auto hash = r2::utils::Hash<const char*>{}(fileName);
    return hash;
}
