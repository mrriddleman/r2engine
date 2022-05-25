//
//  AssetFile.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-23.
//

#ifndef AssetFile_h
#define AssetFile_h

namespace r2::asset
{
    class Asset;
    
    class AssetFile
    {
    public:
        virtual bool Open() = 0;
        virtual bool Close() = 0;
        virtual bool IsOpen() const = 0;
        virtual u64 RawAssetSize(const Asset& asset) = 0;
        virtual u64 LoadRawAsset(const Asset& asset, byte* data, u32 dataBufSize) = 0;
        virtual u64 NumAssets() const = 0;
        virtual void GetAssetName(u64 index, char* name, u32 nameBuferSize) const = 0;
        virtual u64 GetAssetHandle(u64 index) const = 0;
        virtual ~AssetFile() {}
        virtual const char* FilePath() const = 0;

    };
}

#endif /* AssetFile_h */
