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
#include "r2/Core/Math/Transform.h"

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
        glm::mat4 offsetTransform;
		//glm::mat4 offsetTransform = glm::mat4(1.0f);
		//glm::mat4 finalTransform = glm::mat4(1.0f);
	};

	struct ShaderBoneTransform
	{
		glm::mat4 globalInv; //@Memory: could maybe remove or have this be a per object param
		glm::mat4 transform;
		glm::mat4 invBindPose; //@Memory: could maybe remove or have this be a per object param
	};


    struct Skeleton
    {
        r2::SArray<u64>* mJointNames = nullptr;
        r2::SArray<s32>* mParents = nullptr;
        r2::SArray<r2::math::Transform>* mRestPoseTransforms = nullptr; 
        r2::SArray<r2::math::Transform>* mBindPoseTransforms = nullptr;
        r2::SArray<s32>* mRealParentBones = nullptr; //for debug

//#ifdef R2_DEBUG
//        std::vector<std::string> mDebugBoneNames;
//#endif // R2_DEBUG

        static u64 MemorySizeNoData(u64 numJoints, u64 alignment, u32 headerSize, u32 boundsChecking);
    };

	struct Model
	{
		u64 hash = 0;
        
        //these should be the same size
        r2::SArray<MaterialHandle>* optrMaterialHandles = nullptr;
		r2::SArray<const Mesh*>* optrMeshes = nullptr;

        glm::mat4 globalInverseTransform;

		static u64 MemorySize(u64 numMeshes, u64 numVertices, u64 numIndices, u64 headerSize, u64 boundsChecking, u64 alignment);
		static u64 ModelMemorySize(u64 numMeshes, u64 alignment, u32 headerSize, u32 boundsChecking);


	};


	struct AnimModel
	{
        Model model;
		r2::SArray<BoneData>* boneData = nullptr;
		r2::SArray<BoneInfo>* boneInfo = nullptr;

		//r2::SArray<Animation>* animations = nullptr;
 //       r2::SArray<Animation>* noptrAddedAnimations = nullptr;
		r2::SHashMap<s32>* boneMapping = nullptr;
		
        Skeleton skeleton;

        //This will only calculate the amount of memory needed for this object given the inputs WITHOUT calculating the amount of data needed for each individual object of the array(s)
        static u64 MemorySizeNoData(u64 boneMapping, u64 boneDataSize, u64 boneInfoSize, u64 numMeshes, u64 alignment, u32 headerSize, u32 boundsChecking);
		//char directory[r2::fs::FILE_PATH_LENGTH] = { '\0' };
        static u64 MemorySizeNoData(u64 numMeshes, u64 alignment, u32 headerSize, u32 boundsChecking);
	};

	struct DebugBone
	{
		glm::vec3 p0;
		glm::vec3 p1;
	};

	struct MeshRef
	{
		//Where the data lives
		u32 baseVertex = 0;
		u32 baseIndex = 0;
		u32 numIndices = 0;
		u32 numVertices = 0;

        u32 materialIndex = 0;
    };

    struct ModelRef
    {
        u64 hash;
        //@NOTE(Serge): I assume we'll only be uploading mesh data to one vertex buffer and not have them span different ones? If this changes then this should be on the MeshRef
		VertexBufferHandle vertexBufferHandle;
		IndexBufferHandle indexBufferHandle;

        r2::SArray<MeshRef>* mMeshRefs;
        r2::SArray<MaterialHandle>* mMaterialHandles;

        b32 mAnimated;
        u32 mNumBones;
    };

    using ModelRefHandle = s64; //index into the model refs of the renderer currently
    const ModelRefHandle InvalidModelRefHandle = -1;

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
