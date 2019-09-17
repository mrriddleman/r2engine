//
//  AssetCompiler.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-08-25.
//

#ifndef AssetCompiler_h
#define AssetCompiler_h

#ifdef R2_ASSET_PIPELINE
#include <string>
#include "r2/Core/Assets/Pipeline/AssetWatcher.h"

namespace r2::asset::pln
{
    struct AssetManifest;
}

namespace r2::asset::pln::cmp
{
    bool Init(const std::string& tempBuildPath, r2::asset::pln::AssetsBuiltFunc assetsBuiltFunc);
    //Should ignore dups
    void PushBuildRequest(const r2::asset::pln::AssetManifest& manifest);
    void Update();
    
}
#endif
#endif /* AssetCompiler_h */
