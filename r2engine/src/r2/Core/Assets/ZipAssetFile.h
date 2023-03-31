//
//  ZipAssetFile.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-07-31.
//

#ifndef ZipAssetFile_h
#define ZipAssetFile_h

#include "r2/Core/Assets/AssetFile.h"
#include "r2/Core/Memory/Memory.h"

namespace r2::fs
{
    class ZipFile;
}

namespace r2::asset
{
    class R2_API ZipAssetFile: public AssetFile
    {
    public:
        ZipAssetFile();
        ~ZipAssetFile();
        bool Init(const char* assetPath, r2::mem::AllocateFunc alloc, r2::mem::FreeFunc free);
        virtual bool Open(bool writable = false) override;
        virtual bool Open(r2::fs::FileMode mode) override;
        virtual bool Close() override;
        virtual bool IsOpen() const override;
        virtual u64 RawAssetSize(const Asset& asset) override;
        virtual u64 LoadRawAsset(const Asset& asset, byte* data, u32 dataBufSize) override;
        virtual u64 WriteRawAsset(const Asset& asset, byte* data, u32 dataBufferSize) override;
        virtual u64 NumAssets() override;
        virtual void GetAssetName(u64 index, char* name, u32 nameBuferSize) override;
        virtual u64 GetAssetHandle(u64 index) override;
        
        virtual const char* FilePath() const override {return mPath;}
    private:
        char mPath[r2::fs::FILE_PATH_LENGTH];
        r2::fs::ZipFile* mZipFile;
        r2::mem::AllocateFunc mAlloc;
        r2::mem::FreeFunc mFree;
    };
}

#endif /* ZipAssetFile_h */
