//
//  AssetCompiler.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-25.
//

#ifndef AssetCompiler_h
#define AssetCompiler_h

#include <string>

namespace r2::asset::pln
{
    struct AssetManifest;
}

namespace r2::asset::pln::cmp
{
    bool Init(const std::string& tempBuildPath);
    //Should ignore dups
    void PushBuildRequest(const r2::asset::pln::AssetManifest& manifest);
    void Update();
    
}

#endif /* AssetCompiler_h */
