//
//  Model.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#ifndef Model_h
#define Model_h

#include "r2/Render/Model/Mesh.h"
#include "r2/Render/Model/Material.h"
#include "glm/gtc/quaternion.hpp"

#define MAKE_MODEL(arena, numMeshes) r2::draw::MakeModel(arena, numMeshes, __FILE__, __LINE__, "")
#define FREE_MODEL(arena, modelPtr) r2::draw::FreeModel(arena, modelPtr, __FILE__, __LINE__, "")

namespace r2::draw
{
#define MAX_BONE_WEIGHTS 4

	struct BoneData
	{
		glm::vec4 boneWeights = glm::vec4(0.0f);
		glm::ivec4 boneIDs = glm::ivec4(0);
	};

	struct BoneInfo
	{
		glm::mat4 offsetTransform = glm::mat4(1.0f);
		glm::mat4 finalTransform = glm::mat4(1.0f);
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

    struct Skeleton;
    struct Skeleton
    {
        //std::string name;
        u64 hashName;
		glm::mat4 transform = glm::mat4(1.0f);
        Skeleton* parent = nullptr;
		r2::SArray<Skeleton>* children = nullptr;

        static u64 MemorySizeNoData(u64 numChildren, u64 alignment, u32 headerSize, u32 boundsChecking);
    };

	struct AnimationChannel
	{
		//std::string name;
        u64 hashName;
		//u32 numPositionKeys = 0;
		//u32 numRotationKeys = 0;
		//u32 numScalingKeys = 0;
		r2::SArray<VectorKey>* positionKeys = nullptr;
		r2::SArray<VectorKey>* scaleKeys = nullptr;
		r2::SArray<RotationKey>* rotationKeys = nullptr;

        static u64 MemorySizeNoData(u64 numPositionKeys, u64 numScaleKeys, u64 numRotationKeys, u64 alignment, u32 headerSize, u32 boundsChecking);
	};

	struct Animation
	{
		//std::string name;
        u64 hashName;
		double duration = 0; //in ticks
		double ticksPerSeconds = 0;
        r2::SArray<AnimationChannel>* channels;

        static u64 MemorySizeNoData(u64 numChannels, u64 alignment, u32 headerSize, u32 boundsChecking);
	};

	struct AnimModel
	{
        u64 hash = 0;
        r2::SArray<Mesh>* meshes = nullptr;
		r2::SArray<BoneData>* boneData = nullptr;
		r2::SArray<BoneInfo>* boneInfo = nullptr;

		r2::SArray<Animation>* animations = nullptr;
        r2::SArray<Animation>* noptrAddedAnimations = nullptr;
		r2::SHashMap<s32>* boneMapping = nullptr;
		
        Skeleton skeleton;
        glm::mat4 globalInverseTransform = glm::mat4(1.0f);

        //This will only calculate the amount of memory needed for this object given the inputs WITHOUT calculating the amount of data needed for each individual object of the array(s)
        static u64 MemorySizeNoData(u64 boneMapping, u64 boneDataSize, u64 boneInfoSize, u64 numMeshes, u64 numAnimations, u64 alignment, u32 headerSize, u32 boundsChecking);
		//char directory[r2::fs::FILE_PATH_LENGTH] = { '\0' };
	};

    struct Model
    {
        u64 hash = 0;
        r2::SArray<Mesh>* optrMeshes = nullptr;
        glm::mat4 globalInverseTransform = glm::mat4(1.0f);

        static u64 MemorySize(u64 numMeshes, u64 numVertices, u64 numIndices, u64 numTextures, u64 headerSize, u64 boundsChecking, u64 alignment);
        static u64 ModelMemorySize(u64 numMeshes, u64 alignment, u32 headerSize, u32 boundsChecking);
    };

    struct ModelRef
    {
        ConstantBufferHandle vertexHandle = EMPTY_BUFFER;
        ConstantBufferHandle indexHandle = EMPTY_BUFFER;

        u64 hash        = 0; //could remove this
        u64 baseVertex  = 0;
        u64 baseIndex   = 0;
        u64 numIndices  = 0;
        u64 numVertices = 0;
    };

    template<class ARENA>
    Model* MakeModel(ARENA& arena, u64 numMeshes, const char* file, s32 line, const char* description);

    template<class ARENA>
    void FreeModel(ARENA& arena, Model* modelPtr, const char* file, s32 line, const char* description);
}

namespace r2::draw
{
    template<class ARENA>
    Model* MakeModel(ARENA& arena, u64 numMeshes, const char* file, s32 line, const char* description)
    {
        Model* newModel = ALLOC_VERBOSE(Model, arena, file, line, description);
        if (!newModel)
        {
            R2_CHECK(false, "We couldn't allocate a new model!");
            return nullptr;
        }
            
        newModel->optrMeshes = MAKE_SARRAY(arena, Mesh, numMeshes);

        if (!newModel->optrMeshes)
        {
            FREE_VERBOSE(newModel, arena, file, line, description);
            R2_CHECK(false, "Failed to allocate the array for all of the meshes");
            return false;
        }

        return newModel;
    }

    template<class ARENA>
    void FreeModel(ARENA& arena, Model* modelPtr, const char* file, s32 line, const char* description)
    {
        u64 numMeshes = r2::sarr::Size(*modelPtr->optrMeshes);

        for (u64 i = 0; i < numMeshes; ++i)
        {
            FREE_MESH(arena, r2::sarr::At(*modelPtr->optrMeshes, i));
        }

        FREE_VERBOSE(modelPtr->optrMeshes, arena, file, line, description);
        FREE_VERBOSE(modelPtr, arena, file, line, description);
    }

    
}

#endif /* Model_h */
