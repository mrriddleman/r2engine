//
//  AssimpLoadHelpers.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-13.
//

#ifndef AssimpLoadHelpers_h
#define AssimpLoadHelpers_h

namespace r2::draw
{
    struct Model;
    struct SkinnedModel;
    
    void LoadModel(Model& model, const char* filePath);
    void LoadSkinnedModel(SkinnedModel& model, const char* filePath);
}

#endif /* AssimpLoadHelpers_h */
