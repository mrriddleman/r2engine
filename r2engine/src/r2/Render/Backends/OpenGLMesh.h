//
//  OpenGLMesh.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#ifndef OpenGLMesh_h
#define OpenGLMesh_h


#include "r2/Render/Backends/OpenGLBuffer.h"
#include "r2/Render/Model/Textures/Texture.h"

namespace r2::draw
{
    struct Model;
    struct SkinnedModel;
}

namespace r2::draw::opengl
{
    struct OpenGLMesh
    {
        VertexArrayBuffer vertexArray;
        
        size_t numTextures;
        size_t numIndices;
        std::vector<r2::draw::tex::TextureType> types;
        std::vector<u32> texIDs;
        
    };

    std::vector<OpenGLMesh> CreateOpenGLMeshesFromModel(const r2::draw::Model& model);
    std::vector<OpenGLMesh> CreateOpenGLMeshesFromSkinnedModel(const r2::draw::SkinnedModel& model);
}

#endif /* OpenGLMesh_h */
