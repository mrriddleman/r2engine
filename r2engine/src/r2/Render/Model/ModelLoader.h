//
//  AssimpLoadHelpers.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-13.
//

#ifndef __LOAD_HELPERS_H__
#define __LOAD_HELPERS_H__

#include "r2/Core/File/PathUtils.h"
#include "r2/Render/Model/Model_generated.h"
#include "r2/Render/Model/Loaders/Flat/FlatbufferModelLoader.h"

namespace r2::draw
{
    struct Model;
    struct SkinnedModel;
    
    template<class ARENA>
    Model* LoadModel(ARENA& arena, const char* filePath)
    {
        char extension[r2::fs::FILE_PATH_LENGTH];
        bool success = r2::fs::utils::CopyFileExtension(filePath, extension);

        if (success)
        {
            R2_CHECK(false, "Failed to get the extension for the filepath: %s", filePath);
            return nullptr;
        }

        if (strcmp(extension, r2::ModelExtension()) == 0)
        {
            //use the flat version
            Model* model = r2::draw::flat::LoadModel(arena, filePath);
            R2_CHECK(model != nullptr, "Failed to load the model at filepath: %s", filePath);
            return model;
        }
        else
        {
            //use the assimp version
            return nullptr;
        }
    }
    void LoadSkinnedModel(SkinnedModel& model, const char* filePath);
    void AddAnimations(SkinnedModel& model, const char* directory);
}

#endif /* AssimpLoadHelpers_h */
