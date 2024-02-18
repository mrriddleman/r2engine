#include "r2pch.h"

#include "r2/Core/Assets/AssetLoaders/RModelAssetLoader.h"
#include "assetlib/RModel_generated.h"
#include "assetlib/RModelMetaData_generated.h"
#include "assetlib/ModelAsset.h"
#include "assetlib/RAnimation_generated.h"
#include "assetlib/RAnimationMetaData_generated.h"
#include "r2/Core/Assets/AssetFiles/MemoryAssetFile.h"
#include "r2/Render/Model/Model.h"
//#include "r2/Render/Animation/Animation.h"
#include <glm/glm.hpp>
#include "r2/Render/Model/Materials/MaterialHelpers.h"
#include "r2/Core/Math/MathUtils.h"
#include "r2/Render/Animation/TransformTrack.h"
#ifdef R2_EDITOR
#include "r2/Core/File/PathUtils.h"
#endif

#ifdef R2_DEBUG
#include "R2/Utils/Timer.h"
#endif

namespace r2::asset
{
	r2::math::Transform GetTransformFromFlatTransform(const flat::Transform* t)
	{
		r2::math::Transform newTransform;

		newTransform.position = glm::vec3(t->position()->Get(0), t->position()->Get(1), t->position()->Get(2));

		//w is first here
		newTransform.rotation = glm::quat(t->rotation()->Get(3), t->rotation()->Get(0), t->rotation()->Get(1), t->rotation()->Get(2));

		newTransform.scale = glm::vec3(t->scale()->Get(0), t->scale()->Get(1), t->scale()->Get(2));

		return newTransform;
	}

	glm::vec4 GetVec4FromFlatVec4(const flat::Vertex4* v)
	{
		return { v->v()->Get(0), v->v()->Get(1), v->v()->Get(2), v->v()->Get(3) };
	}

	glm::vec3 GetVec3FromFlatVec3(const flat::Vertex3& v)
	{
		return { v.x(), v.y(), v.z() };
	}

	glm::ivec4 GetIVec4FromFlatIVec4(const flat::Vertex4i* v)
	{
		return { v->v()->Get(0), v->v()->Get(1), v->v()->Get(2), v->v()->Get(3) };
	}

	RModelAssetLoader::RModelAssetLoader()
	{

	}

	const char* RModelAssetLoader::GetPattern()
	{
		return flat::RModelExtension();
	}

	AssetType RModelAssetLoader::GetType() const
	{
		return RMODEL;
	}

	bool RModelAssetLoader::ShouldProcess()
	{
		return true;
	}

	u64 RModelAssetLoader::GetLoadedAssetSize(const char* filePath, byte* rawBuffer, u64 size, u64 alignment, u32 header, u32 boundsChecking)
	{
		
		MemoryAssetFile memoryAssetFile{(void*) rawBuffer, size };

		r2::assets::assetlib::load_binaryfile(filePath, memoryAssetFile);

		const auto metaData = r2::assets::assetlib::read_model_meta_data(memoryAssetFile);

		R2_CHECK(metaData != nullptr, "Couldn't get the metaData in GetLoadedAssetSize");

		u32 totalSize = 0;

		u32 meshDataSize = 0;
		
		u32 totalVertices = 0;

		for (flatbuffers::uoffset_t i = 0; i < metaData->meshInfos()->size(); ++i)
		{
			R2_CHECK(metaData->meshInfos()->Get(i)->vertexFormat() == flat::VertexFormat_P32N32UV32T32, "This is the only vertex format we accept right now");

			u32 numVertices = metaData->meshInfos()->Get(i)->vertexBufferSize() / sizeof(r2::draw::Vertex);
			u32 numIndices = metaData->meshInfos()->Get(i)->indexBufferSize() / metaData->meshInfos()->Get(i)->sizeOfAnIndex();

			meshDataSize += r2::draw::Mesh::MemorySize(numVertices, numIndices, alignment, header, boundsChecking);

			totalVertices += numVertices;
		}

		totalSize += meshDataSize;

		u32 numBones = 0;
		u32 numBoneData = 0;
		u32 maxNumChannels = 0;
		u32 numAnimationsInModel = 0;
		u32 numJoints = 0;

		if (metaData->isAnimatedModel())
		{
			numBones = metaData->boneMetaData()->numBoneInfo();
			numBoneData = metaData->boneMetaData()->numBoneData();
			numJoints = metaData->skeletonMetaData()->numJoints();

			R2_CHECK(numBones == numJoints, "I BELIEVE these should always be the same?");
		}

		const auto* animationsMetaData = metaData->animationMetaData();

		if (animationsMetaData)
		{
			const auto numAnimations = animationsMetaData->size();
			numAnimationsInModel = numAnimations;


			r2::mem::utils::MemoryProperties memProperties;
			memProperties.alignment = alignment;
			memProperties.headerSize = header;
			memProperties.boundsChecking = boundsChecking;

			for (u32 i = 0; i < numAnimations; ++i)
			{
				const flat::RAnimationMetaData* animationMetaData = animationsMetaData->Get(i);
				const auto* channelsMetaData = animationMetaData->channelsMetaData();
				const auto numChannels = channelsMetaData->size();

				u64 bytes = 0;

				bytes += r2::anim::AnimationClip::MemorySize(numChannels, memProperties);

				for (u32 j = 0; j < numChannels; ++j)
				{
					const auto channelMetaData = channelsMetaData->Get(j);

					bytes += anim::TransformTrack::MemorySize(
						channelMetaData->numPositionKeys(),
						channelMetaData->numScaleKeys(),
						channelMetaData->numRotationKeys(),
						channelMetaData->numberOfSampledPositionFrames(),
						channelMetaData->numberOfSampledScaleFrames(),
						channelMetaData->numberOfSampledRotationFrames(),
						memProperties
					);
				}

				/*u64 bytes = r2::draw::Animation::MemorySize(numChannels, alignment, header, boundsChecking);

				for (u32 j = 0; j < numChannels; ++j)
				{
					const auto channelMetaData = channelsMetaData->Get(j);
					bytes += r2::draw::AnimationChannel::MemorySizeNoData(
						channelMetaData->numPositionKeys(),
						channelMetaData->numScaleKeys(),
						channelMetaData->numRotationKeys(),
						animationMetaData->durationInTicks(),
						alignment, header, boundsChecking);
				}*/

				totalSize += bytes;
			}
		}
		
		totalSize += r2::draw::Model::MemorySize(metaData->numMeshes(), metaData->numMaterials(), numJoints, totalVertices, numAnimationsInModel, metaData->numGLTFMeshes(), alignment, header, boundsChecking);

		return totalSize;
	}

	bool RModelAssetLoader::LoadAsset(const char* filePath, byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer)
	{

	//	PROFILE_SCOPE("RModelAssetLoader::LoadAsset");

		void* dataPtr = assetBuffer.MutableData();

		void* startOfArrayPtr = nullptr;

		MemoryAssetFile memoryAssetFile{ (void*)rawBuffer, rawSize };

		r2::assets::assetlib::load_binaryfile(filePath, memoryAssetFile);

		const auto metaData = r2::assets::assetlib::read_model_meta_data(memoryAssetFile);

		R2_CHECK(metaData != nullptr, "metaData is null!");

		const auto modelData = r2::assets::assetlib::read_model_data(metaData, memoryAssetFile);

		R2_CHECK(modelData != nullptr, "modelData is null!");

		const auto numMeshes = metaData->numMeshes();
		const auto numVertices = metaData->numVertices();
		const auto numMaterials = metaData->numMaterials();

		
		r2::draw::Model* model = new (dataPtr) r2::draw::Model();

		R2_CHECK(model != nullptr, "We should have a proper model!");

		model->optrBoneData = nullptr;
		//model->optrBoneInfo = nullptr;
		//model->optrBoneMapping = nullptr;
		//model->skeleton.mBindPoseTransforms = nullptr;
		//model->skeleton.mJointNames = nullptr;
		//model->skeleton.mParents = nullptr;
		//model->skeleton.mRealParentBones = nullptr;
		//model->skeleton.mRestPoseTransforms = nullptr;
		//model->optrAnimations = nullptr;

		startOfArrayPtr = r2::mem::utils::PointerAdd(dataPtr, sizeof(r2::draw::Model));

		model->optrMaterialNames = EMPLACE_SARRAY(startOfArrayPtr, r2::mat::MaterialName, numMaterials);

		startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::mat::MaterialName>::MemorySize(numMaterials));

		model->optrMeshes = EMPLACE_SARRAY(startOfArrayPtr, const r2::draw::Mesh*, numMeshes);

		startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::Mesh>::MemorySize(numMeshes));

		const flat::Matrix4* flatGlobalInverse = modelData->globalInverseTransform();

		model->globalInverseTransform = r2::math::GetGLMMatrix4FromFlatMatrix(flatGlobalInverse);
		model->globalTransform = glm::inverse(model->globalInverseTransform);

		r2::asset::MakeAssetNameFromFlatAssetName(metaData->modelAssetName(), model->assetName);

		//materials

		const auto flatMaterialNames = modelData->materials();

		for (flatbuffers::uoffset_t i = 0; i < flatMaterialNames->size(); ++i)
		{
			const auto* flatMaterialName = flatMaterialNames->Get(i);
			R2_CHECK(flatMaterialName->materialPackName() != 0, "The material pack name should never be nullptr");
			r2::sarr::Push(*model->optrMaterialNames, r2::mat::MakeMaterialNameFromFlatMaterial(flatMaterialName));
		}

		const auto flatMeshes = modelData->meshes();

		for (flatbuffers::uoffset_t i = 0; i < flatMeshes->size(); ++i)
		{
		//	PROFILE_SCOPE("RModelAssetLoader::LoadAsset - Load 1 mesh");
			r2::draw::Mesh* nextMeshPtr = new (startOfArrayPtr) r2::draw::Mesh();
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, sizeof(r2::draw::Mesh));

			const auto numVertices = metaData->meshInfos()->Get(i)->vertexBufferSize() / sizeof(r2::draw::Vertex);
			const auto numIndices = metaData->meshInfos()->Get(i)->indexBufferSize() / metaData->meshInfos()->Get(i)->sizeOfAnIndex();

			R2_CHECK(numIndices > 0, "We should have indices for this format");

			nextMeshPtr->optrVertices = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::Vertex, numVertices);
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::Vertex>::MemorySize(numVertices));

			nextMeshPtr->optrIndices = EMPLACE_SARRAY(startOfArrayPtr, u32, numIndices);
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<u32>::MemorySize(numIndices));

			r2::assets::assetlib::unpack_mesh(
				metaData,
				i,
				reinterpret_cast<const char*>(modelData->meshes()->Get(i)->data()->Data()),
				metaData->meshInfos()->Get(i)->compressedSize(),
				reinterpret_cast<char*>(nextMeshPtr->optrVertices->mData),
				reinterpret_cast<char*>(nextMeshPtr->optrIndices->mData));

			nextMeshPtr->optrIndices->mSize = numIndices;
			nextMeshPtr->optrVertices->mSize = numVertices;

			nextMeshPtr->assetName = metaData->meshInfos()->Get(i)->meshName();
			nextMeshPtr->materialIndex = modelData->meshes()->Get(i)->materialIndex();
			const auto bounds = metaData->meshInfos()->Get(i)->meshBounds();

			nextMeshPtr->objectBounds.radius = bounds->radius();
			nextMeshPtr->objectBounds.origin = GetVec3FromFlatVec3(bounds->origin());
			nextMeshPtr->objectBounds.extents = GetVec3FromFlatVec3(bounds->extents());

			r2::sarr::Push(*model->optrMeshes, const_cast<const r2::draw::Mesh*>(nextMeshPtr));
		}

		if (metaData->isAnimatedModel())
		{
			model->optrBoneData = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::BoneData, numVertices);
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::BoneData>::MemorySize(numVertices));

			const auto flatBoneData = modelData->animationData()->boneData();

			for (flatbuffers::uoffset_t i = 0; i < flatBoneData->size(); ++i)
			{
				r2::draw::BoneData boneData;
				boneData.boneWeights = GetVec4FromFlatVec4(&flatBoneData->Get(i)->boneWeights());
				boneData.boneIDs = GetIVec4FromFlatIVec4(&flatBoneData->Get(i)->boneIDs());

				r2::sarr::Push(*model->optrBoneData, boneData);
			}

			model->animSkeleton = anim::LoadSkeleton(&startOfArrayPtr, modelData->animationData());
		}

		const auto* animationMetaData = metaData->animationMetaData();
		
		if (animationMetaData && animationMetaData->size() > 0)
		{
			R2_CHECK(modelData->animations()->size() == animationMetaData->size(), "These should always be the same");
			const auto numAnimations = modelData->animations()->size();
			const auto* animations = modelData->animations();

			model->optrAnimationClips = EMPLACE_SARRAY(startOfArrayPtr, r2::anim::AnimationClip*, numAnimations);
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::anim::AnimationClip*>::MemorySize(numAnimations));
			for (u32 i = 0; i < numAnimations; ++i)
			{
				const flat::RAnimation* flatAnimationData = animations->Get(i);
				r2::anim::AnimationClip* nextClip = anim::LoadAnimationClip(&startOfArrayPtr, flatAnimationData);
				r2::sarr::Push(*model->optrAnimationClips, nextClip);
			}
		}

		u32 numGLTFMeshes = metaData->numGLTFMeshes();
		model->optrGLTFMeshInfos = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::GLTFMeshInfo, numGLTFMeshes);
		startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::GLTFMeshInfo>::MemorySize(numGLTFMeshes));

		const auto flatGLTFMeshInfoData = modelData->gltfMeshInfos();

		for (u32 i=0; i < numGLTFMeshes; ++i)
		{
			r2::draw::GLTFMeshInfo gltfMeshInfo;
			const auto flatGLTFMeshInfo = flatGLTFMeshInfoData->Get(i);
			
			gltfMeshInfo.numPrimitives = flatGLTFMeshInfo->numPrimitives();
			gltfMeshInfo.meshGlobal = r2::math::GetGLMMatrix4FromFlatMatrix(flatGLTFMeshInfo->globalTransform());
			gltfMeshInfo.meshGlobalInv = r2::math::GetGLMMatrix4FromFlatMatrix(flatGLTFMeshInfo->globalInverseTransform());

			r2::sarr::Push(*model->optrGLTFMeshInfos, gltfMeshInfo);
		}

		return true;
	}

	bool RModelAssetLoader::FreeAsset(const AssetBuffer& assetBuffer)
	{
		return true;
	}

}