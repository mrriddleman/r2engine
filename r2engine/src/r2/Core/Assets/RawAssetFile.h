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
        bool Init(const char* path);
        virtual bool Open() override;
        virtual bool Close() override;
        virtual bool IsOpen() override;
        virtual u64 RawAssetSize(const r2::asset::Asset& asset) override;
        virtual u64 GetRawAsset(const r2::asset::Asset& asset, byte* data, u32 dataBufSize) override;
        virtual u64 NumAssets() const override;
        virtual void GetAssetName(u64 index, char* name, u32 nameBuferSize) const override;
        virtual u64 GetAssetHandle(u64 index) const override;
    private:
        char mPath[r2::fs::FILE_PATH_LENGTH];
        r2::fs::File* mFile;
    };
}

#endif /* RawAssetFile_h */
