//
//  Model.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#include "Model.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "r2/Core/File/PathUtils.h"
#include "r2/Render/Renderer/RendererUtils.h"

namespace
{
    std::vector<r2::draw::Texture> s_loadedTextures;
    void ProcessNode(r2::draw::Model& model, aiNode* node, const aiScene* scene);
    r2::draw::Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, const char* directory);
    std::vector<r2::draw::Texture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, r2::draw::TextureType r2Type, const char* directory);
}

namespace r2::draw
{
    void LoadModel(Model& model, const char* filePath)
    {
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);
        
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            R2_CHECK(false, "Failed to load model: %s", import.GetErrorString());
            return;
        }
        
        r2::fs::utils::CopyDirectoryOfFile(filePath, model.directory);
        ProcessNode(model, scene->mRootNode, scene);
    }
}

namespace
{
    void ProcessNode(r2::draw::Model& model, aiNode* node, const aiScene* scene)
    {
        //process all of the node's meshes
        for (u32 i = 0; i < node->mNumMeshes; ++i)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            model.meshes.push_back(ProcessMesh(mesh, scene, model.directory));
        }
        //then do the same for each of its children
        for(u32 i = 0; i < node->mNumChildren; ++i)
        {
            ProcessNode(model, node->mChildren[i], scene);
        }
    }
    
    r2::draw::Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, const char* directory)
    {
        r2::draw::Mesh aMesh;
        
        for (u32 i = 0; i < mesh->mNumVertices; ++i)
        {
            r2::draw::Vertex v;
            glm::vec3 position;
            position.x = mesh->mVertices[i].x;
            position.y = mesh->mVertices[i].y;
            position.z = mesh->mVertices[i].z;
            v.position = position;
            
            glm::vec3 normal;
            normal.x = mesh->mNormals[i].x;
            normal.y = mesh->mNormals[i].y;
            normal.z = mesh->mNormals[i].z;
            v.normal = normal;
            
            if (mesh->mTextureCoords[0])
            {
                glm::vec2 tex;
                tex.x = mesh->mTextureCoords[0][i].x;
                tex.y = mesh->mTextureCoords[0][i].y;
                v.texCoords = tex;
            }
            else
            {
                v.texCoords = glm::vec2(0);
            }
            
            aMesh.vertices.push_back(v);
        }
        
        for (u32 i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];
            for (u32 j = 0; j < face.mNumIndices; ++j)
            {
                aMesh.indices.push_back(face.mIndices[j]);
            }
        }
        
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            std::vector<r2::draw::Texture> diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, r2::draw::TextureType::Diffuse, directory);
            aMesh.textures.insert(aMesh.textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            
            std::vector<r2::draw::Texture> specularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, r2::draw::TextureType::Specular, directory);
            aMesh.textures.insert(aMesh.textures.end(), specularMaps.begin(), specularMaps.end());
        }
        
        return aMesh;
    }
    
    std::vector<r2::draw::Texture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, r2::draw::TextureType r2Type, const char* directory)
    {
        std::vector<r2::draw::Texture> textures = {};
        
        for (u32 i = 0; i < aiGetMaterialTextureCount(mat, type); ++i)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            
            bool skip = false;
            for (u32 j = 0; j < s_loadedTextures.size(); ++j)
            {
                if(std::strcmp(s_loadedTextures[j].path, str.C_Str()) == 0)
                {
                    textures.push_back(s_loadedTextures[j]);
                    skip = true;
                    break;
                }
            }
            
            if (!skip)
            {
                r2::draw::Texture texture;
                char path[r2::fs::FILE_PATH_LENGTH];
                r2::fs::utils::AppendSubPath(directory, path, str.C_Str());
                texture.texID = r2::draw::utils::LoadImageTexture(path);
                texture.type = r2Type;
                strcpy(texture.path, str.C_Str());
                textures.push_back(texture);
                s_loadedTextures.push_back(texture);
            }
        }
        
        return textures;
    }
}
