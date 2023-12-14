//
//  Model.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//

#ifndef Model_h
#define Model_h

#include "r2/Render/Model/Mesh.h"
#include "r2/Core/Math/Transform.h"
#include "r2/Render/Model/Materials/MaterialTypes.h"


namespace r2::draw
{
	struct Animation;

#define MAX_BONE_WEIGHTS 4

	struct BoneData
	{
		glm::vec4 boneWeights = glm::vec4(0.0f);
		glm::ivec4 boneIDs = glm::ivec4(0);
	};

	struct BoneInfo
	{
        glm::mat4 offsetTransform;
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
		r2::asset::AssetName assetName;
        
        //these should be the same size
		r2::SArray<const Mesh*>* optrMeshes = nullptr;
        r2::SArray<r2::mat::MaterialName>* optrMaterialNames = nullptr;
		r2::SArray<BoneData>* optrBoneData = nullptr;
		r2::SArray<BoneInfo>* optrBoneInfo = nullptr;
		r2::SHashMap<s32>* optrBoneMapping = nullptr;
		
		Skeleton skeleton;

		r2::SArray<Animation*>* optrAnimations = nullptr;

        glm::mat4 globalInverseTransform;

		static u64 ModelMemorySize(u64 numMeshes, u64 numMaterials, u64 alignment, u32 headerSize, u32 boundsChecking);

		static u64 MemorySize(u32 numMeshes, u32 numMaterials, u32 numJoints, u32 boneDataSize, u32 boneInfoSize, u32 numAnimations, u32 alignment, u32 headerSize, u32 boundsChecking);
	};

	//struct AnimModel
	//{
 //       Model model;
	//	r2::SArray<BoneData>* boneData = nullptr;
	//	r2::SArray<BoneInfo>* boneInfo = nullptr;

	//	//r2::SArray<Animation>* animations = nullptr;
 ////       r2::SArray<Animation>* noptrAddedAnimations = nullptr;
	//	r2::SHashMap<s32>* boneMapping = nullptr;
	//	
 //       Skeleton skeleton;

 //       //This will only calculate the amount of memory needed for this object given the inputs WITHOUT calculating the amount of data needed for each individual object of the array(s)
 //       static u64 MemorySizeNoData(u64 boneMapping, u64 boneDataSize, u64 boneInfoSize, u64 numMeshes, u64 numMaterials, u64 alignment, u32 headerSize, u32 boundsChecking);
	//	//char directory[r2::fs::FILE_PATH_LENGTH] = { '\0' };
 //       static u64 MemorySizeNoData(u64 numMeshes, u64 numMaterials, u64 alignment, u32 headerSize, u32 boundsChecking);
	//};

	struct DebugBone
	{
		glm::vec3 p0;
		glm::vec3 p1;
	};
}


#endif /* Model_h */
