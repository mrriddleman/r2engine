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
        mName = "";
#endif
    }
    
    Asset::Asset(const char* name, r2::asset::AssetType type)
    {

#ifdef R2_ASSET_CACHE_DEBUG
        mName = name;
#endif
        char path[r2::fs::FILE_PATH_LENGTH];
        r2::util::PathCpy(path, name);

        std::transform(std::begin(path), std::end(path), std::begin(path), (int(*)(int))std::tolower);
        
        mHashedPathID = STRING_ID(path);
        mType = type;
    }

    Asset::Asset(u64 hash, r2::asset::AssetType type)
        : mHashedPathID(hash)
        , mType(type)
    {
#ifdef R2_ASSET_CACHE_DEBUG
        mName[0] = '\0';
#endif
    }
    
    Asset::Asset(const Asset& asset)
    {
#ifdef R2_ASSET_CACHE_DEBUG
        mName = asset.mName;
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
        mName = asset.mName;
#endif
        mHashedPathID = asset.mHashedPathID;
        mType = asset.mType;
        return *this;
    }

    Asset Asset::MakeAssetFromFilePath(const char* filePath, r2::asset::AssetType type)
    {
        char assetName[r2::fs::FILE_PATH_LENGTH];
        MakeAssetNameStringForFilePath(filePath, assetName, type);
        Asset newAsset(assetName, type);
        return newAsset;
    }

    u64 Asset::GetAssetNameForFilePath(const char* filePath, r2::asset::AssetType type)
    {
        Asset newAsset = MakeAssetFromFilePath(filePath, type);
        return newAsset.HashID();
    }
}
