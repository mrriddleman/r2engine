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
#include "r2/Render/Animation/Skeleton.h"
#include "r2/Render/Animation/AnimationClip.h"

namespace r2::draw
{
//	struct Animation;

	struct BoneData
	{
		glm::vec4 boneWeights = glm::vec4(0.0f);
		glm::ivec4 boneIDs = glm::ivec4(0);
	};

	//struct BoneInfo
	//{
 //       glm::mat4 offsetTransform;
	//};

	struct ShaderBoneTransform
	{
		glm::mat4 transform;
		glm::mat4 invBindPose; //@Memory: could maybe remove or have this be a per object param
	};


//    struct Skeleton
//    {
//        r2::SArray<u64>* mJointNames = nullptr;
//        r2::SArray<s32>* mParents = nullptr;
//        r2::SArray<r2::math::Transform>* mRestPoseTransforms = nullptr; 
//        r2::SArray<r2::math::Transform>* mBindPoseTransforms = nullptr;
//        r2::SArray<s32>* mRealParentBones = nullptr; //for debug
//
////#ifdef R2_DEBUG
////        std::vector<std::string> mDebugBoneNames;
////#endif // R2_DEBUG
//
//        static u64 MemorySizeNoData(u64 numJoints, u64 alignment, u32 headerSize, u32 boundsChecking);
//    };

	struct GLTFMeshInfo
	{
		u32 numPrimitives;
		glm::mat4 meshGlobalInv;
		glm::mat4 meshGlobal;
	};

	struct Model
	{
		r2::asset::AssetName assetName;
        
        //these should be the same size
		r2::SArray<const Mesh*>* optrMeshes = nullptr;
        r2::SArray<r2::mat::MaterialName>* optrMaterialNames = nullptr;
		r2::SArray<BoneData>* optrBoneData = nullptr;
	//	r2::SArray<BoneInfo>* optrBoneInfo = nullptr; //@TODO(Serge): Remove
	//	r2::SHashMap<s32>* optrBoneMapping = nullptr; //@TODO(Serge): Remove
		
	//	Skeleton skeleton; //@TODO(Serge): Remove
		anim::Skeleton animSkeleton;
		r2::SArray<anim::AnimationClip*>* optrAnimationClips = nullptr;

	//	r2::SArray<Animation*>* optrAnimations = nullptr; //@TODO(Serge): Remove

		r2::SArray<GLTFMeshInfo>* optrGLTFMeshInfos = nullptr;

        glm::mat4 globalInverseTransform; 
		glm::mat4 globalTransform;

		static u64 ModelMemorySize(u64 numMeshes, u64 numMaterials, u64 alignment, u32 headerSize, u32 boundsChecking);

		static u64 MemorySize(u32 numMeshes, u32 numMaterials, u32 numJoints, u32 boneDataSize, u32 numAnimations, u32 numGLTFMeshes, u32 alignment, u32 headerSize, u32 boundsChecking);
	};

	struct DebugBone
	{
		glm::vec3 p0;
		glm::vec3 p1;
	};
}


#endif /* Model_h */
