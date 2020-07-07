//
//  RawAssetFile.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-07-17.
//

#ifndef RawAssetFile_h
#define RawAssetFile_h

#include "r2/Core/Assets/AssetFile.h"
#include "r2/Core/File/File.h"

namespace r2::asset
{
    class RawAssetFile: public AssetFile {
    public:
        RawAssetFile();
        ~RawAssetFile();
        bool Init(const char* path, u32 numDirectoriesToIncludeInAssetHandle = 0);
        virtual bool Open() override;
        virtual bool Close() override;
        virtual bool IsOpen() const override;
        virtual u64 RawAssetSize(const r2::asset::Asset& asset) override;
        virtual u64 GetRawAsset(const r2::asset::Asset& asset, byte* data, u32 dataBufSize) override;
        virtual u64 NumAssets() const override;
        virtual void GetAssetName(u64 index, char* name, u32 nameBuferSize) const override;
        virtual u64 GetAssetHandle(u64 index) const override;
        virtual const char* FilePath() const override {return mPath;}
    private:
        char mPath[r2::fs::FILE_PATH_LENGTH];
        r2::fs::File* mFile;
        u32 mNumDirectoriesToIncludeInAssetHandle;
    };
}

#endif /* RawAssetFile_h */
