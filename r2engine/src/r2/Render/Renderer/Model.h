//
//  Model.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#ifndef Model_h
#define Model_h

#include "r2/Render/Renderer/Mesh.h"

namespace r2::draw
{
    struct Model
    {
        std::vector<Mesh> meshes;
        glm::mat4 globalInverseTransform;
        char directory[r2::fs::FILE_PATH_LENGTH];
    };
}

#endif /* Model_h */
