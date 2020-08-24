//
//  Asset.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-20.
//
#include "r2pch.h"
#include "Asset.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"
#include "r2/Platform/Platform.h"

namespace r2::asset
{
    Asset::Asset()
    : mHashedPathID(0)
    , mType(DEFAULT)
    {
#ifdef R2_ASSET_CACHE_DEBUG
        r2::util::PathCpy(mName, "");
#endif
    }
    
    Asset::Asset(const char* name, r2::asset::AssetType type)
    {

#ifdef R2_ASSET_CACHE_DEBUG
        r2::util::PathCpy(mName, name);
#endif
        char path[r2::fs::FILE_PATH_LENGTH];
        r2::util::PathCpy(path, name);

        std::transform(std::begin(path), std::end(path), std::begin(path), (int(*)(int))std::tolower);
        
        mHashedPathID = r2::utils::Hash<const char*>{}(path);
        mType = type;
    }

    Asset::Asset(u64 hash, r2::asset::AssetType type)
        : mHashedPathID(hash)
        , mType(type)
    {
    }
    
    Asset::Asset(const Asset& asset)
    {
#ifdef R2_ASSET_CACHE_DEBUG
        r2::util::PathCpy(mName, asset.mName);
#endif
        mHashedPathID = asset.mHashedPathID;
        mType = asset.mType;
    }
    
    Asset& Asset::operator=(const Asset& asset)
    {
        if (this == &asset)
        {
            return *this;
        }
#ifdef R2_ASSET_CACHE_DEBUG
        r2::util::PathCpy(mName, asset.mName);
#endif
        mHashedPathID = asset.mHashedPathID;
        mType = asset.mType;
        return *this;
    }
}
