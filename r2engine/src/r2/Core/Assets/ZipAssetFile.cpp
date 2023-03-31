//
//  ZipAssetFile.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-07-31.
//
#include "r2pch.h"
#include "r2/Core/Assets/ZipAssetFile.h"
#include "r2/Core/File/FileDevices/Modifiers/Zip/ZipFile.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"

namespace r2::asset
{
    ZipAssetFile::ZipAssetFile():mZipFile(nullptr)
    {
        r2::util::PathCpy(mPath, "");
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
        R2_CHECK(alloc != nullptr, "Cannot pass in nullptr for alloc function!");
        R2_CHECK(free != nullptr, "Cannot pass in nullptr for free function!");
        R2_CHECK(assetPath != nullptr, "Cannot pass in nullptr for asset path!");
        R2_CHECK(assetPath != "", "Do not pass in empty path!");
        
        r2::util::PathCpy(mPath, assetPath);

        

        mAlloc = alloc;
        mFree = free;
        mZipFile = nullptr;
        return strcmp(mPath, "") != 0;
    }
    
    bool ZipAssetFile::Open(bool writable)
    {
        r2::fs::FileMode mode = r2::fs::Mode::Read | r2::fs::Mode::Binary;
        if (writable)
        {
            mode |= r2::fs::Mode::Write;
        }

        return Open(mode);
    }

    bool ZipAssetFile::Open(r2::fs::FileMode mode)
    {
		r2::fs::DeviceConfig zipConfig;
		zipConfig.AddModifier(r2::fs::DeviceModifier::Zip);

		mZipFile = (r2::fs::ZipFile*)r2::fs::FileSystem::Open(zipConfig, mPath, mode);

		R2_CHECK(mZipFile != nullptr, "We should have a zip file!");

		bool result = mZipFile->InitArchive(mAlloc, mFree);

		R2_CHECK(result, "We should be able to initialize the archive!");

		return result;
    }
    
    bool ZipAssetFile::Close()
    {
        if (mZipFile == nullptr)
        {
            return true;
        }
        r2::fs::FileSystem::Close(mZipFile);
        mZipFile = nullptr;
        return true;
    }
    
    bool ZipAssetFile::IsOpen() const
    {
        return mZipFile && mZipFile->IsOpen();
    }
    
    u64 ZipAssetFile::RawAssetSize(const Asset& asset)
    {
        u32 fileIndex = 0;
        bool found = mZipFile->FindFile(asset.HashID(), fileIndex);
        
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
    
    u64 ZipAssetFile::LoadRawAsset(const Asset& asset, byte* data, u32 dataBufSize)
    {
		if (!(mZipFile && mZipFile->IsOpen()))
		{
			return 0;
		}

        return mZipFile->ReadUncompressedFileDataByHash(data, dataBufSize, asset.HashID());
    }
    
    u64 ZipAssetFile::WriteRawAsset(const Asset& asset, byte* data, u32 dataBufferSize)
    {
        //@TODO(Serge): implement
        return 0;
    }

    u64 ZipAssetFile::NumAssets() 
    {
        bool opened = false;
        if (!mZipFile || !mZipFile->IsOpen())
        {
			opened = Open();
			R2_CHECK(opened, "We couldn't open the zip file: %s\n", mPath);
        }

        auto numFiles =  mZipFile->GetNumberOfFiles();

        if (opened)
        {
            Close();
        }

        return numFiles;
    }
    
    void ZipAssetFile::GetAssetName(u64 index, char* name, u32 nameBuferSize)
    {
		bool opened = false;
		if (!mZipFile || !mZipFile->IsOpen())
		{
			opened = Open();
			R2_CHECK(opened, "We couldn't open the zip file: %s\n", mPath);
		}

        mZipFile->GetFilename(static_cast<u32>( index), name, nameBuferSize);

		if (opened)
		{
			Close();
		}
    }
    
    u64 ZipAssetFile::GetAssetHandle(u64 index)
    {
        char fileName[r2::fs::FILE_PATH_LENGTH];
        GetAssetName(index, fileName, r2::fs::FILE_PATH_LENGTH);

        std::transform(std::begin(fileName), std::end(fileName), std::begin(fileName), (int(*)(int))std::tolower);

        return STRING_ID(fileName);
    }
}
