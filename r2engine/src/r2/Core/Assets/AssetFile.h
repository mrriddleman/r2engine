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
        virtual bool IsOpen() = 0;
        virtual u64 RawAssetSize(const Asset& asset) = 0;
        virtual u64 RawAsset(const Asset& asset, byte* data) = 0;
        virtual u64 NumAssets() const = 0;
        virtual char* GetAssetName(u64 index) const = 0;
        virtual ~AssetFile() {}
    };
}

#endif /* AssetFile_h */
