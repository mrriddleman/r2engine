//
//  Asset.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-20.
//

#ifndef Asset_h
#define Asset_h

#include "r2/Core/Assets/AssetTypes.h"

namespace r2::asset
{
    class R2_API Asset
    {
    public:
        
        Asset();
        Asset(const char* name, r2::asset::AssetType type);
        Asset(u64 hash, r2::asset::AssetType type);
        Asset(const Asset& asset);
        Asset& operator=(const Asset& asset);
        

        inline bool Empty() const { return mHashedPathID == 0; }
        
#ifdef R2_ASSET_CACHE_DEBUG
        inline const char* Name() const {return mName;}
#endif
        inline u64 HashID() const {return mHashedPathID;}
        inline AssetType GetType() const { return mType; }
    private:
        #ifdef R2_ASSET_CACHE_DEBUG
        char mName[r2::fs::FILE_PATH_LENGTH]; 
        #endif

        u64 mHashedPathID;
        AssetType mType;
    };
}

#endif /* Asset_h */
