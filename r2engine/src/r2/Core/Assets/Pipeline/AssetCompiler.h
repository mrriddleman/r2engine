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

namespace r2::asset::pln
{
    struct AssetManifest;
    using AssetsBuiltFunc = std::function<void(std::vector<std::string> paths)>;
}

namespace r2::asset::pln::cmp
{
    

    bool Init(const std::string& tempBuildPath, r2::asset::pln::AssetsBuiltFunc assetsBuiltFunc);
    //Should ignore dups
    void PushBuildRequest(const r2::asset::pln::AssetManifest& manifest);
    void Update();
    
    bool CompileAsset(const r2::asset::pln::AssetManifest& manifest);
}
#endif
#endif /* AssetCompiler_h */
