//
//  AssetFile.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-23.
//

#ifndef AssetFile_h
#define AssetFile_h
#include "r2/Core/File/FileTypes.h"

namespace r2::asset
{
    class Asset;
    
    class AssetFile
    {
    public:
        virtual bool Open(bool writable=false) = 0;
        virtual bool Open(r2::fs::FileMode mode) = 0;
        virtual bool Close() = 0;
        virtual bool IsOpen() const = 0;
        virtual u64 RawAssetSize(const Asset& asset) = 0;
        virtual u64 LoadRawAsset(const Asset& asset, byte* data, u32 dataBufSize) = 0;
        virtual u64 WriteRawAsset(const Asset& asset, const byte* data, u32 dataBufferSize, u32 offset) = 0;
        virtual u64 NumAssets() = 0;
        virtual void GetAssetName(u64 index, char* name, u32 nameBuferSize) = 0;
        virtual u64 GetAssetHandle(u64 index) = 0;
        virtual ~AssetFile() {}
        virtual const char* FilePath() const = 0;
        
    };
}

#endif /* AssetFile_h */
