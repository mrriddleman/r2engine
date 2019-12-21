//
//  OpenGLMesh.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#include "OpenGLMesh.h"
#include "glad/glad.h"
#include "r2/Render/Renderer/Model.h"
#include "r2/Render/Renderer/SkinnedModel.h"

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
        
//        glEnableVertexAttribArray(3);
//        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));
//        
//        glEnableVertexAttribArray(4);
//        glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIDs));
        
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
    
    std::vector<OpenGLMesh> CreateOpenGLMeshesFromSkinnedModel(const r2::draw::SkinnedModel& model)
    {
        std::vector<OpenGLMesh> meshes = {};
        
        u32 numEntries = model.meshEntries.size();
        
        for (u32 i = 0; i < numEntries; ++i)
        {
            OpenGLMesh glMesh;
            
            const r2::draw::MeshEntry& entry = model.meshEntries[i];
            glGenVertexArrays(1, &glMesh.VAO);
            glGenBuffers(1, &glMesh.VBO);
            glGenBuffers(1, &glMesh.EBO);
            glGenBuffers(1, &glMesh.BoneVBO);
            glGenBuffers(1, &glMesh.BoneEBO);
            
            //VBO setup
            glBindVertexArray(glMesh.VAO);
            glBindBuffer(GL_ARRAY_BUFFER, glMesh.VBO);
            glBufferData(GL_ARRAY_BUFFER, entry.numVertices * sizeof(Vertex), &model.mesh.vertices[entry.baseVertex], GL_STATIC_DRAW);
            
            
            //vertex positions
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            //vertex normals
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
            //vertex texture coords
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
            
            glBindBuffer(GL_ARRAY_BUFFER, glMesh.BoneVBO);
            glBufferData(GL_ARRAY_BUFFER, entry.numVertices * sizeof(BoneData), &model.boneDataVec[entry.baseVertex], GL_STATIC_DRAW);
            
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(BoneData), (void*)offsetof(BoneData, boneWeights));

            glEnableVertexAttribArray(4);
            glVertexAttribIPointer(4, 4, GL_INT, sizeof(BoneData), (void*)offsetof(BoneData, boneIDs));
            
            //EBO setup
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMesh.EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, entry.numIndices * sizeof(u32), &model.mesh.indices[entry.baseIndex], GL_STATIC_DRAW);
            
            glBindVertexArray(0);
            
            //setup the rest of the state
            glMesh.numIndices = entry.numIndices;
            
            glMesh.numTextures = entry.textures.size();
            for (u32 i = 0; i < glMesh.numTextures; ++i)
            {
                glMesh.types.push_back(entry.textures[i].type);
                glMesh.texIDs.push_back(entry.textures[i].texID);
            }
            meshes.push_back(glMesh);
        }
        
        //HACK!!!!!!
//        std::sort(meshes.begin(), meshes.end(), [](const opengl::OpenGLMesh& mesh1, const opengl::OpenGLMesh& mesh2)
//        {
//            return mesh1.numIndices > mesh2.numIndices;
//        });
        
        return meshes;
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
