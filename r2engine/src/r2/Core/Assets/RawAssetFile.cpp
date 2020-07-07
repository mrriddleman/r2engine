//
//  RawAssetFile.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-07-17.
//
#include "r2pch.h"
#include "r2/Core/Assets/RawAssetFile.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"

namespace r2::asset
{
    RawAssetFile::RawAssetFile(): mFile(nullptr), mNumDirectoriesToIncludeInAssetHandle(0)
    {
        r2::util::PathCpy(mPath, "");
    }
    
    RawAssetFile::~RawAssetFile()
    {
        if (mFile)
        {
            Close();
        }
    }
    
    bool RawAssetFile::Init(const char* path, u32 numDirectoriesToIncludeInAssetHandle)
    {
        if (!path)
        {
            return false;
        }
        mNumDirectoriesToIncludeInAssetHandle = numDirectoriesToIncludeInAssetHandle;
        r2::util::PathCpy(mPath, path);

        return strcmp(mPath, "") != 0;
    }
    
    bool RawAssetFile::Open()
    {
        r2::fs::DeviceConfig config;
        r2::fs::FileMode mode;
        mode = r2::fs::Mode::Read;
        mode |= r2::fs::Mode::Binary;
        
        mFile = r2::fs::FileSystem::Open(config, mPath, mode);
        
        return mFile != nullptr;
    }
    
    bool RawAssetFile::Close()
    {
        r2::fs::FileSystem::Close(mFile);
        mFile = nullptr;
        return true;
    }
    
    bool RawAssetFile::IsOpen() const
    {
        return mFile != nullptr;
    }
    
    u64 RawAssetFile::RawAssetSize(const r2::asset::Asset& asset)
    {
        return mFile->Size();
    }
    
    u64 RawAssetFile::GetRawAsset(const r2::asset::Asset& asset, byte* data, u32 dataBufSize)
    {
        return mFile->ReadAll(data);
    }
    
    u64 RawAssetFile::NumAssets() const
    {
        return 1;
    }
    
    void RawAssetFile::GetAssetName(u64 index, char* name, u32 nameBuferSize) const
    {
        r2::fs::utils::CopyFileNameWithParentDirectories(mPath, name, mNumDirectoriesToIncludeInAssetHandle);
    }
    
    u64 RawAssetFile::GetAssetHandle(u64 index) const
    {
        char fileName[r2::fs::FILE_PATH_LENGTH];
        r2::fs::utils::CopyFileNameWithParentDirectories(mPath, fileName, mNumDirectoriesToIncludeInAssetHandle);
       
        std::transform(std::begin(fileName), std::end(fileName), std::begin(fileName), (int(*)(int))std::tolower);

        auto hash = STRING_ID(fileName);
        return hash;
    }
}
