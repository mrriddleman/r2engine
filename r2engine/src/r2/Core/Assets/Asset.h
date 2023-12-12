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
        inline const std::string& Name() const {return mName;}
#endif


        //@TODO(Serge): UUID - This should return our AssetName type not a hash
        inline u64 HashID() const {return mHashedPathID;}
        inline AssetType GetType() const { return mType; }

        static Asset MakeAssetFromFilePath(const char* filePath, r2::asset::AssetType type);
        static u64 GetAssetNameForFilePath(const char* filePath, r2::asset::AssetType type);

    private:
        #ifdef R2_ASSET_CACHE_DEBUG
        std::string mName; 
        #endif

        u64 mHashedPathID;
        AssetType mType;
    };



}

#endif /* Asset_h */
