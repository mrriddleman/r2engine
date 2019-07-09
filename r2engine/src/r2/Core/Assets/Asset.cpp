//
//  Asset.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-20.
//

#include "Asset.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"
#include "r2/Platform/Platform.h"

namespace r2::asset
{
    Asset::Asset()
    : mHashedPathID(0)
    {
        strcpy(mName, "");
    }
    
    Asset::Asset(const char* name)
    {
        bool result = false;

        strcpy(mName, name);
        result = true;
        
        R2_CHECK(result, "Asset::Asset() - couldn't append path");
        
        std::transform(std::begin(mName), std::end(mName), std::begin(mName), (int(*)(int))std::tolower);
        
        mHashedPathID = r2::utils::Hash<const char*>{}(mName);
    }
    
    Asset::Asset(const Asset& asset)
    {
        strcpy(mName, asset.mName);
        mHashedPathID = asset.mHashedPathID;
    }
    
    Asset& Asset::operator=(const Asset& asset)
    {
        if (this == &asset)
        {
            return *this;
        }
        
        strcpy(mName, asset.mName);
        mHashedPathID = asset.mHashedPathID;
        return *this;
    }
}
