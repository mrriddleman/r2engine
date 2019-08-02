//
//  ZipAssetFile.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-07-31.
//

#include "r2/Core/Assets/ZipAssetFile.h"
#include "r2/Core/File/FileDevices/Modifiers/Zip/ZipFile.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"

namespace r2::asset
{
    ZipAssetFile::ZipAssetFile():mZipFile(nullptr)
    {
        strcpy(mPath, "");
    }
    
    ZipAssetFile::~ZipAssetFile()
    {
        if (IsOpen())
        {
            Close();
        }
    }
    
    bool ZipAssetFile::Init(const char* assetPath, r2::mem::AllocateFunc alloc, r2::mem::FreeFunc free)
    {
        r2::fs::DeviceConfig zipConfig;
        zipConfig.AddModifier(r2::fs::DeviceModifier::Zip);
        
        mZipFile = (r2::fs::ZipFile*)r2::fs::FileSystem::Open(zipConfig, assetPath, r2::fs::Mode::Read | r2::fs::Mode::Binary);
        
        R2_CHECK(mZipFile != nullptr, "We should have a zip file!");
        
        bool result = mZipFile->InitArchive(alloc, free);
        
        R2_CHECK(result, "We should be able to initialize the archive!");
        
        return result;
    }
    
    bool ZipAssetFile::Open()
    {
        return mZipFile != nullptr;
    }
    
    bool ZipAssetFile::Close()
    {
        r2::fs::FileSystem::Close(mZipFile);
        mZipFile = nullptr;
        return true;
    }
    
    bool ZipAssetFile::IsOpen()
    {
        return mZipFile && mZipFile->IsOpen();
    }
    
    u64 ZipAssetFile::RawAssetSize(const Asset& asset)
    {
        u32 fileIndex = 0;
        bool found = mZipFile->FindFile(asset.Name(), fileIndex);
        
        if (found)
        {
            r2::fs::ZipFile::ZipFileInfo info;
            bool foundExtra;
            bool gotInfo = mZipFile->GetFileInfo(fileIndex, info, &foundExtra);
            
            if (gotInfo)
            {
                return info.uncompressedSize;
            }
        }
        
        return 0;
    }
    
    u64 ZipAssetFile::GetRawAsset(const Asset& asset, byte* data, u32 dataBufSize)
    {
        return mZipFile->ReadUncompressedFileData(data, dataBufSize, asset.Name());
    }
    
    u64 ZipAssetFile::NumAssets() const
    {
        return mZipFile->GetNumberOfFiles();
    }
    
    void ZipAssetFile::GetAssetName(u64 index, char* name, u32 nameBuferSize) const
    {
        mZipFile->GetFilename(index, name, nameBuferSize);
    }
    
    u64 ZipAssetFile::GetAssetHandle(u64 index) const
    {
        char fileName[r2::fs::FILE_PATH_LENGTH];
        GetAssetName(index, fileName, r2::fs::FILE_PATH_LENGTH);
        return r2::utils::Hash<const char*>{}(fileName);
    }
}
