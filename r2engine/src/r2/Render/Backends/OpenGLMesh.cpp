//
//  OpenGLMesh.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//
#include "r2pch.h"
#include "OpenGLMesh.h"
#include "glad/glad.h"
//#include "r2/Render/Renderer/Model.h"
#include "r2/Render/Renderer/SkinnedModel.h"

namespace r2::draw::opengl
{
    void CreateOpenGLMesh(OpenGLMesh& glMesh, const r2::draw::Mesh& aMesh)
    {
        Create(glMesh.vertexArray);
        VertexBuffer vertexBuffer;
        //Create(vertexBuffer, {
        //    {ShaderDataType::Float3, "aPos"},
        //    {ShaderDataType::Float3, "aNormal"},
        //    {ShaderDataType::Float2, "aTexCoord"},
        //    //{ShaderDataType::Float3, "aTangent"},
        //}, aMesh.vertices, GL_STATIC_DRAW);
        
        IndexBuffer indexBuffer;
      //  Create(indexBuffer, aMesh.indices.data(), aMesh.indices.size(), GL_STATIC_DRAW);
        
        //AddBuffer(glMesh.vertexArray, vertexBuffer);
        SetIndexBuffer(glMesh.vertexArray, indexBuffer);
        UnBind(glMesh.vertexArray);
        
        
        //setup the rest of the state
     //   glMesh.numIndices = aMesh.indices.size();
    //    glMesh.numTextures = aMesh.textures.size();
        for (u32 i = 0; i < glMesh.numTextures; ++i)
        {
     //       glMesh.types.push_back(aMesh.textures[i].type);
     //       glMesh.texIDs.push_back(aMesh.textures[i].texID);
        }
    }
    
    std::vector<OpenGLMesh> CreateOpenGLMeshesFromSkinnedModel(const r2::draw::SkinnedModel& model)
    {
        std::vector<OpenGLMesh> meshes = {};
        
        size_t numEntries = model.meshEntries.size();
        
        for (u32 i = 0; i < numEntries; ++i)
        {
            OpenGLMesh glMesh;
            const r2::draw::MeshEntry& entry = model.meshEntries[i];
            
            Create(glMesh.vertexArray);
            VertexBuffer meshVBO;
            //Create(meshVBO, {
            //    {ShaderDataType::Float3, "aPos"},
            //    {ShaderDataType::Float3, "aNormal"},
            //    {ShaderDataType::Float2, "aTexCoord"},
            //    //{ShaderDataType::Float3, "aTangent"},
            //}, &model.mesh.vertices[entry.baseVertex], entry.numVertices, GL_STATIC_DRAW);
            
       //     AddBuffer(glMesh.vertexArray, meshVBO);
            
            //VertexBuffer boneVBO;
            //Create(boneVBO, {
            //    {ShaderDataType::Float4, "aBoneWeights"},
            //    {ShaderDataType::Int4, "aBoneIDs"}
            //}, &model.boneDataVec[entry.baseVertex], entry.numVertices, GL_STATIC_DRAW);
            
         //   AddBuffer(glMesh.vertexArray, boneVBO);
            
            IndexBuffer meshIBO;
        //    Create(meshIBO, &model.mesh.indices[entry.baseIndex], entry.numIndices, GL_STATIC_DRAW);
            
            SetIndexBuffer(glMesh.vertexArray, meshIBO);
            
            UnBind(glMesh.vertexArray);
            
            //setup the rest of the state
            glMesh.numIndices = entry.numIndices;
            
            glMesh.numTextures = entry.textures.size();
            for (u32 i = 0; i < glMesh.numTextures; ++i)
            {
              //  glMesh.types.push_back(entry.textures[i].type);
             //   glMesh.texIDs.push_back(entry.textures[i].texID);
            }
            meshes.push_back(glMesh);
        }
        
        return meshes;
    }
    
    std::vector<OpenGLMesh> CreateOpenGLMeshesFromModel(const r2::draw::Model& model)
    {
        std::vector<OpenGLMesh> meshes = {};
        
 //       for (u32 i = 0; i < model.meshes.size(); ++i)
        {
            OpenGLMesh openGLMesh;
//            CreateOpenGLMesh(openGLMesh, model.meshes[i]);
            
            meshes.push_back(openGLMesh);
        }
        
        return meshes;
    }
}
