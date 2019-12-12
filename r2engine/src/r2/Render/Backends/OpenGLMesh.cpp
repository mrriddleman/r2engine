//
//  OpenGLMesh.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#include "OpenGLMesh.h"
#include "glad/glad.h"
#include "r2/Render/Renderer/Model.h"

namespace r2::draw::opengl
{
    void CreateOpenGLMesh(OpenGLMesh& glMesh, const r2::draw::Mesh& aMesh)
    {
        glGenVertexArrays(1, &glMesh.VAO);
        glGenBuffers(1, &glMesh.VBO);
        glGenBuffers(1, &glMesh.EBO);
        //VBO setup
        glBindVertexArray(glMesh.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, glMesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, aMesh.vertices.size() * sizeof(Vertex), &aMesh.vertices[0], GL_STATIC_DRAW);
        //EBO setup
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, aMesh.indices.size() * sizeof(u32), &aMesh.indices[0], GL_STATIC_DRAW);
        //vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        //vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        //vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
        
        glBindVertexArray(0);
        
        //setup the rest of the state
        glMesh.numIndices = aMesh.indices.size();
        glMesh.numTextures = aMesh.textures.size();
        for (u32 i = 0; i < glMesh.numTextures; ++i)
        {
            glMesh.types.push_back(aMesh.textures[i].type);
            glMesh.texIDs.push_back(aMesh.textures[i].texID);
        }
    }
    
    std::vector<OpenGLMesh> CreateOpenGLMeshesFromModel(const r2::draw::Model& model)
    {
        std::vector<OpenGLMesh> meshes = {};
        
        for (u32 i = 0; i < model.meshes.size(); ++i)
        {
            OpenGLMesh openGLMesh;
            CreateOpenGLMesh(openGLMesh, model.meshes[i]);
            meshes.push_back(openGLMesh);
        }
        
        return meshes;
    }
}
