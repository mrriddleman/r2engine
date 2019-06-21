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
    Asset::Asset(u32 category, const char* name)
    {
        char categoryPath[r2::fs::FILE_PATH_LENGTH];
  
        PathResolver pathResolver = CPLAT.GetPathResolver();
        
        pathResolver(category, categoryPath);
        
        bool result = false;
        if (strcmp(categoryPath, "") != 0)
        {
            result = r2::fs::utils::AppendSubPath(categoryPath, mPath, name, r2::fs::utils::PATH_SEPARATOR);
        }
        else
        {
            strcpy(mPath, name);
            result = true;
        }
        
        R2_CHECK(result, "Asset::Asset() - couldn't append path");
        
        std::transform(std::begin(mPath), std::end(mPath), std::begin(mPath), (int(*)(int))std::tolower);
        
        mHashedPathID = r2::utils::Hash<const char*>{}(mPath);
    }
}
