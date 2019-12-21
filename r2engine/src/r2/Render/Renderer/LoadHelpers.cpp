//
//  AssimpLoadHelpers.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-13.
//

#include "LoadHelpers.h"

#include "r2/Core/File/PathUtils.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Math/MathUtils.h"

#include "r2/Render/Renderer/Vertex.h"
#include "r2/Render/Renderer/Model.h"
#include "r2/Render/Renderer/SkinnedModel.h"
#include "r2/Render/Renderer/RendererUtils.h"
#include "r2/Render/Renderer/Model.h"


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace
{
    std::vector<r2::draw::Texture> s_loadedTextures;
}

namespace
{
    std::vector<r2::draw::Texture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, r2::draw::TextureType r2Type, const char* directory);
    
    void ProcessNode(r2::draw::Model& model, aiNode* node, const aiScene* scene);
    void ProcessNode(r2::draw::SkinnedModel& model, aiNode* node, r2::draw::SkeletonPart& parentSkeletonPart, const aiScene* scene, u32& numVertices, u32& numIndices);
    
    r2::draw::Mesh ProcessMesh(const aiMesh* mesh, const aiScene* scene, const char* directory);
    void ProcessBones(r2::draw::SkinnedModel& model, const aiMesh* mesh, const aiScene* scene);
    void ProcessAnimations(r2::draw::SkinnedModel& model, const aiNode* node, const aiScene* scene);
    
    glm::mat4 AssimpMat4ToGLMMat4(const aiMatrix4x4& mat);
    glm::vec3 AssimpVec3ToGLMVec3(const aiVector3D& vec3D);
    glm::quat AssimpQuatToGLMQuat(const aiQuaternion& quat);
}

namespace r2::draw
{
    void LoadModel(Model& model, const char* filePath)
    {
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(filePath, aiProcess_Triangulate | aiProcess_GenSmoothNormals );
        
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            R2_CHECK(false, "Failed to load model: %s", import.GetErrorString());
            return;
        }
        
        r2::fs::utils::CopyDirectoryOfFile(filePath, model.directory);
        model.globalInverseTransform = glm::inverse(AssimpMat4ToGLMMat4(scene->mRootNode->mTransformation));
        ProcessNode(model, scene->mRootNode, scene);
        
        import.FreeScene();
    }
    
    void LoadSkinnedModel(SkinnedModel& model, const char* filePath)
    {
        
        Assimp::Importer import;
        
        const aiScene* scene = import.ReadFile(filePath, //aiProcess_CalcTangentSpace |
                                               //aiProcess_JoinIdenticalVertices |
                                               aiProcess_Triangulate |
                                              // aiProcess_SortByPType | // ?
                                               aiProcess_GenSmoothNormals |
                                               aiProcess_ImproveCacheLocality |
                                               aiProcess_RemoveRedundantMaterials |
                                              // aiProcess_FindDegenerates |
                                               //aiProcess_GenUVCoords |
                                               //aiProcess_TransformUVCoords |
                                              // aiProcess_FindInstances |
                                               aiProcess_LimitBoneWeights |
                                               aiProcess_OptimizeMeshes |
                                               aiProcess_SplitByBoneCount
                                               );
        
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            R2_CHECK(false, "Failed to load model: %s", import.GetErrorString());
            return;
        }
        
        r2::fs::utils::CopyDirectoryOfFile(filePath, model.directory);
        model.globalInverseTransform = glm::inverse(AssimpMat4ToGLMMat4(scene->mRootNode->mTransformation));//goes from mesh space -> model space
        
        model.skeleton.name = std::string(scene->mRootNode->mName.data);
        model.skeleton.transform = AssimpMat4ToGLMMat4(scene->mRootNode->mTransformation);
        model.skeleton.parent = nullptr;
        u32 numVertices = 0;
        u32 numIndices = 0;
        ProcessNode(model, scene->mRootNode, model.skeleton, scene, numVertices, numIndices);
        ProcessAnimations(model, scene->mRootNode, scene);
        
        import.FreeScene();
    }
    
    void AddAnimations(SkinnedModel& model, const char* directory)
    {
        Assimp::Importer import;
        
        r2::SArray<char[r2::fs::FILE_PATH_LENGTH]>* fileList = nullptr;
        
        fileList = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, char[r2::fs::FILE_PATH_LENGTH], 100);
        
        r2::fs::FileSystem::CreateFileListFromDirectory(directory, ".fbx", fileList);
        
        u64 size = r2::sarr::Size(*fileList);
        
        for (u64 i = 0; i < size; ++i)
        {
            const char* animName = r2::sarr::At(*fileList, i);
            
            const aiScene* newAnim = import.ReadFile(animName, 0);
            ProcessAnimations(model, newAnim->mRootNode, newAnim);
            import.FreeScene();
        }
        
        FREE(fileList, *MEM_ENG_SCRATCH_PTR);
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
    
    void ProcessNode(r2::draw::SkinnedModel& model, aiNode* node, r2::draw::SkeletonPart& parentSkeletonPart, const aiScene* scene, u32& numVertices, u32& numIndices)
    {
        //process all of the node's meshes
        for (u32 i = 0; i < node->mNumMeshes; ++i)
        {
            u32 meshID = node->mMeshes[i];
            aiMesh* mesh = scene->mMeshes[meshID];
            
            model.meshEntries.emplace_back(r2::draw::MeshEntry());
            r2::draw::MeshEntry& entry = model.meshEntries.back();
            entry.numIndices = mesh->mNumFaces * 3;
            entry.baseVertex = numVertices;
            entry.baseIndex = numIndices;
            entry.numVertices = mesh->mNumVertices;
            
            numVertices += mesh->mNumVertices;
            numIndices += entry.numIndices;
            
            r2::draw::Mesh r2Mesh = ProcessMesh(mesh, scene, model.directory);
            
            //add all the vertices to the end of the vector
            model.mesh.vertices.insert(model.mesh.vertices.end(), r2Mesh.vertices.begin(), r2Mesh.vertices.end());
            model.mesh.indices.insert(model.mesh.indices.end(), r2Mesh.indices.begin(), r2Mesh.indices.end());
            
            entry.textures.insert(model.mesh.textures.end(), r2Mesh.textures.begin(), r2Mesh.textures.end());
            
            model.boneDataVec.resize(model.mesh.vertices.size(), r2::draw::BoneData());
            
            ProcessBones(model, mesh, scene);
        }
        
        //then do the same for each of its children
        for(u32 i = 0; i < node->mNumChildren; ++i)
        {
            r2::draw::SkeletonPart child;
            child.name = std::string(node->mChildren[i]->mName.data);
            child.transform = AssimpMat4ToGLMMat4(node->mChildren[i]->mTransformation);
            child.parent = &parentSkeletonPart;
            parentSkeletonPart.children.push_back(child);
            
            ProcessNode(model, node->mChildren[i], parentSkeletonPart.children.back(), scene, numVertices, numIndices);
        }
    }
    
    r2::draw::Mesh ProcessMesh(const aiMesh* mesh, const aiScene* scene, const char* directory)
    {
        r2::draw::Mesh r2Mesh;
        r2Mesh.vertices.reserve(mesh->mNumVertices);
        r2Mesh.indices.reserve(mesh->mNumFaces * 3);
        
        for (u32 i = 0; i < mesh->mNumVertices; ++i)
        {
            r2::draw::Vertex v;
            glm::vec3 position;
            position.x = mesh->mVertices[i].x;
            position.y = mesh->mVertices[i].y;
            position.z = mesh->mVertices[i].z;
            v.position = position;
            
            if(mesh->mNormals)
            {
                glm::vec3 normal;
                normal.x = mesh->mNormals[i].x;
                normal.y = mesh->mNormals[i].y;
                normal.z = mesh->mNormals[i].z;
                v.normal = normal;
            }
            
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
            
            r2Mesh.vertices.push_back(v);
        }
        
        for (u32 i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];
            for (u32 j = 0; j < face.mNumIndices; ++j)
            {
                r2Mesh.indices.push_back(face.mIndices[j]);
            }
        }

        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            R2_CHECK(material != nullptr, "Material is null!");
            if (material)
            {
                std::vector<r2::draw::Texture> diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, r2::draw::TextureType::Diffuse, directory);
                r2Mesh.textures.insert(r2Mesh.textures.end(), diffuseMaps.begin(), diffuseMaps.end());
                
                std::vector<r2::draw::Texture> specularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, r2::draw::TextureType::Specular, directory);
                r2Mesh.textures.insert(r2Mesh.textures.end(), specularMaps.begin(), specularMaps.end());
            }
        }
        
        return r2Mesh;
    }
    
    std::vector<r2::draw::Texture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, r2::draw::TextureType r2Type, const char* directory)
    {
        std::vector<r2::draw::Texture> textures = {};
        const u32 numTextures = aiGetMaterialTextureCount(mat, type);
        for (u32 i = 0; i < numTextures; ++i)
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
    
    void ProcessBones(r2::draw::SkinnedModel& model, const aiMesh* mesh, const aiScene* scene)
    {
        for (u32 i = 0; i < mesh->mNumBones; ++i)
        {
            u32 boneIndex = 0;
            std::string boneName{mesh->mBones[i]->mName.data};
            
            if (model.boneMapping.find(boneName) == model.boneMapping.end())
            {
                boneIndex = model.boneInfos.size();
                r2::draw::BoneInfo info;
                model.boneInfos.push_back(info);
                
                model.boneMapping[boneName] = boneIndex;
                model.boneInfos[boneIndex].offsetTransform = AssimpMat4ToGLMMat4( mesh->mBones[i]->mOffsetMatrix);
            }
            else
            {
                boneIndex = model.boneMapping[boneName];
            }

            size_t numWeights = mesh->mBones[i]->mNumWeights;
            
            for (u32 j = 0; j < numWeights; ++j)
            {
                u32 vertexID = model.meshEntries.back().baseVertex + mesh->mBones[i]->mWeights[j].mVertexId;
                
                bool found = false;
                
                float currentWeightTotal = 0.0f;
                for (u32 k = 0; k < MAX_BONE_WEIGHTS; ++k)
                {
                    currentWeightTotal += model.boneDataVec[vertexID].boneWeights[k];
                    
                    if (!r2::math::NearEq(currentWeightTotal, 1.0f) && r2::math::NearEq(model.boneDataVec[vertexID].boneWeights[k],0.0f))
                    {
                        model.boneDataVec[vertexID].boneIDs[k] = boneIndex;
                        model.boneDataVec[vertexID].boneWeights[k] = mesh->mBones[i]->mWeights[j].mWeight;
                        
                        found = true;
                        break;
                    }
                }
                
                //R2_CHECK(found, "We couldn't find a spot for the bone weight!");
            }
        }
    }
    
    void ProcessAnimations(r2::draw::SkinnedModel& model, const aiNode* node, const aiScene* scene)
    {
        if(scene->HasAnimations())
        {
            u32 oldSize = model.animations.size();
            u32 startingIndex = oldSize;
            u32 newSize = oldSize + scene->mNumAnimations;
            model.animations.resize(newSize);
            
            for (u32 i = startingIndex; i < newSize; ++i)
            {
                aiAnimation* anim = scene->mAnimations[i - oldSize];
                r2::draw::Animation& r2Anim = model.animations[i];
                r2Anim.duration = anim->mDuration;
                r2Anim.ticksPerSeconds = anim->mTicksPerSecond;
                r2Anim.name = std::string(anim->mName.data);
                
                for (u32 j = 0; j < anim->mNumChannels; ++j)
                {
                    aiNodeAnim* animChannel = anim->mChannels[j];
                    r2::draw::AnimationChannel channel;
                    channel.name = std::string(animChannel->mNodeName.data);
                    
                    channel.numPositionKeys = animChannel->mNumPositionKeys;
                    channel.numScalingKeys = animChannel->mNumScalingKeys;
                    channel.numRotationKeys = animChannel->mNumRotationKeys;
                    
                    for (u32 pKey = 0; pKey < channel.numPositionKeys; ++pKey)
                    {
                        channel.positionKeys.push_back({animChannel->mPositionKeys[pKey].mTime, AssimpVec3ToGLMVec3( animChannel->mPositionKeys[pKey].mValue)});
                    }
                    
                    for (u32 sKey = 0; sKey < channel.numScalingKeys; ++sKey)
                    {
                        channel.scaleKeys.push_back({animChannel->mScalingKeys[sKey].mTime, AssimpVec3ToGLMVec3( animChannel->mScalingKeys[sKey].mValue)});
                    }
                    
                    for (u32 rKey = 0; rKey < channel.numRotationKeys; ++rKey)
                    {
                        channel.rotationKeys.push_back({animChannel->mRotationKeys[rKey].mTime, AssimpQuatToGLMQuat( animChannel->mRotationKeys[rKey].mValue.Normalize())});
                    }
                    
                    r2Anim.channels.push_back(channel);
                }
                
            }
        }
    }
    
    glm::mat4 AssimpMat4ToGLMMat4(const aiMatrix4x4& mat)
    {
        //result[col][row]
        //mat[row][col] a, b, c, d - rows
        //              1, 2, 3, 4 - cols
        glm::mat4 result = glm::mat4();
        
        result[0][0]=mat.a1; result[1][0]=mat.a2; result[2][0]=mat.a3; result[3][0]=mat.a4;
        result[0][1]=mat.b1; result[1][1]=mat.b2; result[2][1]=mat.b3; result[3][1]=mat.b4;
        result[0][2]=mat.c1; result[1][2]=mat.c2; result[2][2]=mat.c3; result[3][2]=mat.c4;
        result[0][3]=mat.d1; result[1][3]=mat.d2; result[2][3]=mat.d3; result[3][3]=mat.d4;
        
        return result;
    }
    
    glm::vec3 AssimpVec3ToGLMVec3(const aiVector3D& vec3D)
    {
        return glm::vec3(vec3D.x, vec3D.y, vec3D.z);
    }
    
    glm::quat AssimpQuatToGLMQuat(const aiQuaternion& quat)
    {
        return glm::quat(quat.w, quat.x, quat.y, quat.z);
    }
}
