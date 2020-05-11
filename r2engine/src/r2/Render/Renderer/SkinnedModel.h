//
//  SkinnedModel.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-13.
//

#ifndef SkinnedModel_h
#define SkinnedModel_h

#include "r2/Render/Model/Mesh.h"
#include "r2/Render/Renderer/Vertex.h"
#include "glm/gtc/quaternion.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace r2::draw
{
    struct BoneInfo
    {
        glm::mat4 offsetTransform;
        glm::mat4 finalTransform;
    };
    
    struct MeshEntry
    {
        u32 numIndices = 0;
        u32 numVertices = 0;
        u32 baseVertex = 0;
        u32 baseIndex = 0;
        std::vector<Texture> textures;
    };
    
    struct VectorKey
    {
        double time;
        glm::vec3 value;
    };
    
    struct RotationKey
    {
        double time;
        glm::quat quat;
    };
    
    struct AnimationChannel
    {
        std::string name;
        u32 numPositionKeys;
        u32 numRotationKeys;
        u32 numScalingKeys;
        std::vector<VectorKey> positionKeys;
        std::vector<VectorKey> scaleKeys;
        std::vector<RotationKey> rotationKeys;
    };
    
    struct Animation
    {
        std::string name;
        double duration = 0; //in ticks
        double ticksPerSeconds = 0;
        std::vector<AnimationChannel> channels;
    };
    
    struct SkeletonPart;
    struct SkeletonPart
    {
        std::string name;
        glm::mat4 transform = glm::mat4(1.0f);
        SkeletonPart* parent = nullptr;
        std::vector<SkeletonPart> children;
    };
    
    struct SkinnedModel
    {
        Mesh mesh;
        std::unordered_map<std::string, u32> boneMapping;
        std::vector<BoneData> boneDataVec;
        std::vector<BoneInfo> boneInfos;
        std::vector<MeshEntry> meshEntries;
        glm::mat4 globalInverseTransform = glm::mat4(1.0f);
        std::vector<Animation> animations;
        SkeletonPart skeleton;
        char directory[r2::fs::FILE_PATH_LENGTH] = {'\0'};
    };
}

#endif /* SkinnedModel_h */
