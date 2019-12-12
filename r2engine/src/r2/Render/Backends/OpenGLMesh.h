//
//  OpenGLMesh.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#ifndef OpenGLMesh_h
#define OpenGLMesh_h

#include "r2/Render/Renderer/Mesh.h"

namespace r2::draw
{
    struct Model;
}

namespace r2::draw::opengl
{
    struct OpenGLMesh
    {
        u32 VAO, VBO, EBO;
        
        u32 numTextures;
        u32 numIndices;
        std::vector<TextureType> types;
        std::vector<u32> texIDs;
    };
    
    void CreateOpenGLMesh(OpenGLMesh& glMesh, const r2::draw::Mesh& aMesh);
    std::vector<OpenGLMesh> CreateOpenGLMeshesFromModel(const r2::draw::Model& model);
}

#endif /* OpenGLMesh_h */
