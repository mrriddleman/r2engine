#include "r2pch.h"

#include "r2/Core/Assets/AssetLoaders/RModelAssetLoader.h"
#include "assetlib/RModel_generated.h"
#include "assetlib/RModelMetaData_generated.h"
#include "assetlib/ModelAsset.h"
#include "assetlib/RAnimation_generated.h"
#include "assetlib/RAnimationMetaData_generated.h"
#include "r2/Core/Assets/AssetFiles/MemoryAssetFile.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Animation/Animation.h"
#include <glm/glm.hpp>
#include "r2/Render/Model/Materials/MaterialHelpers.h"
#ifdef R2_EDITOR
#include "r2/Core/File/PathUtils.h"
#endif

namespace r2::asset
{
	glm::mat4 GetGLMMatrix4FromFlatMatrix(const flat::Matrix4* mat)
	{
		glm::mat4 glmMat;
		
		glmMat[0] = glm::vec4(mat->cols()->Get(0)->v()->Get(0), mat->cols()->Get(0)->v()->Get(1), mat->cols()->Get(0)->v()->Get(2), mat->cols()->Get(0)->v()->Get(3));
		glmMat[1] = glm::vec4(mat->cols()->Get(1)->v()->Get(0), mat->cols()->Get(1)->v()->Get(1), mat->cols()->Get(1)->v()->Get(2), mat->cols()->Get(1)->v()->Get(3));
		glmMat[2] = glm::vec4(mat->cols()->Get(2)->v()->Get(0), mat->cols()->Get(2)->v()->Get(1), mat->cols()->Get(2)->v()->Get(2), mat->cols()->Get(2)->v()->Get(3));
		glmMat[3] = glm::vec4(mat->cols()->Get(3)->v()->Get(0), mat->cols()->Get(3)->v()->Get(1), mat->cols()->Get(3)->v()->Get(2), mat->cols()->Get(3)->v()->Get(3));

		return glmMat;
	}

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
		}

		const auto* animationsMetaData = metaData->animationMetaData();

		if (animationsMetaData)
		{
			const auto numAnimations = animationsMetaData->size();
			numAnimationsInModel = numAnimations;

			for (u32 i = 0; i < numAnimations; ++i)
			{
				const flat::RAnimationMetaData* animationMetaData = animationsMetaData->Get(i);
				const auto* channelsMetaData = animationMetaData->channelsMetaData();
				const auto numChannels = channelsMetaData->size();

				u64 bytes = r2::draw::Animation::MemorySize(numChannels, alignment, header, boundsChecking);

				for (u32 j = 0; j < numChannels; ++j)
				{
					const auto channelMetaData = channelsMetaData->Get(j);
					bytes += r2::draw::AnimationChannel::MemorySizeNoData(
						channelMetaData->numPositionKeys(),
						channelMetaData->numScaleKeys(),
						channelMetaData->numRotationKeys(),
						animationMetaData->durationInTicks(),
						alignment, header, boundsChecking);
				}

				totalSize += bytes;
			}
		}
		
		totalSize += r2::draw::Model::MemorySize(metaData->numMeshes(), metaData->numMaterials(), numJoints, totalVertices, numBones, numAnimationsInModel, alignment, header, boundsChecking);

		//if (metaData->isAnimatedModel())
		//{
		//	totalSize += r2::draw::Skeleton::MemorySizeNoData(metaData->skeletonMetaData()->numJoints(), alignment, header, boundsChecking);
		//	
		//	const auto numBones = metaData->boneMetaData()->numBoneInfo();

		//	u64 hashCapacity = static_cast<u64>(std::round(numBones * r2::SHashMap<u32>::LoadFactorMultiplier()));

		//	totalSize += r2::draw::AnimModel::MemorySizeNoData(hashCapacity, totalVertices, numBones, metaData->numMeshes(), metaData->numMaterials(), alignment, header, boundsChecking);

		//	return static_cast<u64>(totalSize);
		//}
		//
		//totalSize += r2::draw::Model::ModelMemorySize(metaData->numMeshes(), metaData->numMaterials(), alignment, header, boundsChecking);

		return totalSize;
	}

	bool RModelAssetLoader::LoadAsset(const char* filePath, byte* rawBuffer, u64 rawSize, AssetBuffer& assetBuffer)
	{
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
		model->optrBoneInfo = nullptr;
		model->optrBoneMapping = nullptr;
		model->skeleton.mBindPoseTransforms = nullptr;
		model->skeleton.mJointNames = nullptr;
		model->skeleton.mParents = nullptr;
		model->skeleton.mRealParentBones = nullptr;
		model->skeleton.mRestPoseTransforms = nullptr;
		model->optrAnimations = nullptr;

		startOfArrayPtr = r2::mem::utils::PointerAdd(dataPtr, sizeof(r2::draw::Model));

		model->optrMaterialNames = EMPLACE_SARRAY(startOfArrayPtr, r2::mat::MaterialName, numMaterials);

		startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::mat::MaterialName>::MemorySize(numMaterials));

		model->optrMeshes = EMPLACE_SARRAY(startOfArrayPtr, const r2::draw::Mesh*, numMeshes);

		startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::Mesh>::MemorySize(numMeshes));

		const flat::Matrix4* flatGlobalInverse = modelData->globalInverseTransform();

		model->globalInverseTransform = GetGLMMatrix4FromFlatMatrix(flatGlobalInverse);

		model->assetName = metaData->modelName();

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
			const auto numBones = metaData->boneMetaData()->numBoneInfo();

			model->optrBoneData = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::BoneData, numVertices);
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::BoneData>::MemorySize(numVertices));

			model->optrBoneInfo = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::BoneInfo, numBones);
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::BoneInfo>::MemorySize(numBones));

			u64 hashCapacity = static_cast<u64>(std::round(numBones * r2::SHashMap<u32>::LoadFactorMultiplier()));

			model->optrBoneMapping = MAKE_SHASHMAP_IN_PLACE(s32, startOfArrayPtr, hashCapacity);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SHashMap<u32>::MemorySize(hashCapacity));

			////Process the Nodes
			const u64 numJoints = metaData->skeletonMetaData()->numJoints();
			const u64 numBindPoseTransforms = metaData->skeletonMetaData()->numBindPoseTransforms();

			model->skeleton.mJointNames = EMPLACE_SARRAY(startOfArrayPtr, u64, numJoints);
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<u64>::MemorySize(numJoints));

			model->skeleton.mParents = EMPLACE_SARRAY(startOfArrayPtr, s32, numJoints);
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<s32>::MemorySize(numJoints));

			model->skeleton.mRestPoseTransforms = EMPLACE_SARRAY(startOfArrayPtr, r2::math::Transform, numJoints);
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::math::Transform>::MemorySize(numJoints));

			model->skeleton.mBindPoseTransforms = EMPLACE_SARRAY(startOfArrayPtr, r2::math::Transform, numBindPoseTransforms);
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::math::Transform>::MemorySize(numBindPoseTransforms));

			model->skeleton.mRealParentBones = EMPLACE_SARRAY(startOfArrayPtr, s32, numBindPoseTransforms);
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<s32>::MemorySize(numBindPoseTransforms));


			//Animation data
			const auto flatBoneInfos = modelData->animationData()->boneInfo();

			for (flatbuffers::uoffset_t i = 0; i < flatBoneInfos->size(); ++i)
			{
				r2::draw::BoneInfo boneInfo;
				boneInfo.offsetTransform = GetGLMMatrix4FromFlatMatrix(&flatBoneInfos->Get(i)->offsetTransform());

				r2::sarr::Push(*model->optrBoneInfo, boneInfo);
			}

			const auto flatBoneData = modelData->animationData()->boneData();

			for (flatbuffers::uoffset_t i = 0; i < flatBoneData->size(); ++i)
			{
				r2::draw::BoneData boneData;
				boneData.boneWeights = GetVec4FromFlatVec4(&flatBoneData->Get(i)->boneWeights());
				boneData.boneIDs = GetIVec4FromFlatIVec4(&flatBoneData->Get(i)->boneIDs());

				r2::sarr::Push(*model->optrBoneData, boneData);
			}

			const auto flatBoneMapEntries = modelData->animationData()->boneMapping();

			for (flatbuffers::uoffset_t i = 0; i < flatBoneMapEntries->size(); ++i)
			{
				r2::shashmap::Set(*model->optrBoneMapping, flatBoneMapEntries->Get(i)->key(), flatBoneMapEntries->Get(i)->val());
			}

			const auto flatJoints = modelData->animationData()->jointNames();

			for (flatbuffers::uoffset_t i = 0; i < flatJoints->size(); ++i)
			{
				r2::sarr::Push(*model->skeleton.mJointNames, flatJoints->Get(i));
			}

			const auto flatParents = modelData->animationData()->parents();

			for (flatbuffers::uoffset_t i = 0; i < flatParents->size(); ++i)
			{
				r2::sarr::Push(*model->skeleton.mParents, flatParents->Get(i));
			}

			const auto flatRestPoseTransforms = modelData->animationData()->restPoseTransforms();

			for (flatbuffers::uoffset_t i = 0; i < flatRestPoseTransforms->size(); ++i)
			{
				r2::sarr::Push(*model->skeleton.mRestPoseTransforms, GetTransformFromFlatTransform(flatRestPoseTransforms->Get(i)));
			}

			const auto flatBindPoseTransforms = modelData->animationData()->bindPoseTransforms();
			const auto flatBindPoseTransformsSize = flatBindPoseTransforms->size();

			for (flatbuffers::uoffset_t i = 0; i < flatBindPoseTransformsSize; ++i)
			{
				r2::sarr::Push(*model->skeleton.mBindPoseTransforms, GetTransformFromFlatTransform(flatBindPoseTransforms->Get(i)));
			}

			const auto flatRealParents = modelData->animationData()->realParentBones();

			for (flatbuffers::uoffset_t i = 0; i < flatRealParents->size(); ++i)
			{
				r2::sarr::Push(*model->skeleton.mRealParentBones, flatRealParents->Get(i));
			}
		}

		const auto* animationMetaData = metaData->animationMetaData();
		
		if (animationMetaData && animationMetaData->size() > 0)
		{
			R2_CHECK(modelData->animations()->size() == animationMetaData->size(), "These should always be the same");
			
			const auto* animations = modelData->animations();

			const auto numAnimations = modelData->animations()->size();

			model->optrAnimations = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::Animation*, numAnimations);
			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::Animation*>::MemorySize(numAnimations));

#ifdef R2_EDITOR
			char animationName[r2::fs::FILE_PATH_LENGTH];
			char sanitizedAnimationPath[r2::fs::FILE_PATH_LENGTH];
#endif

			for (u32 i = 0; i < numAnimations; ++i)
			{
				const flat::RAnimation* flatAnimationData = animations->Get(i);

				r2::draw::Animation* animation = new (startOfArrayPtr) r2::draw::Animation();

				R2_CHECK(animation != nullptr, "Big problem here");

				startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, sizeof(r2::draw::Animation));

#ifdef R2_EDITOR
				//add in the name of the animation here
				const auto* tempAnimationMetaData = animationMetaData->Get(i);
				std::string originalAnimationPath = tempAnimationMetaData->originalPath()->str();
				r2::fs::utils::SanitizeSubPath(originalAnimationPath.c_str(), sanitizedAnimationPath);
				r2::fs::utils::CopyFileName(sanitizedAnimationPath, animationName);
				animation->animationName = animationName;
#endif

				const auto* flatChannels = flatAnimationData->channels();
				const auto numChannels = flatChannels->size();

				const auto hashMapSize = glm::round( numChannels * r2::SHashMap<r2::draw::AnimationChannel>::LoadFactorMultiplier() );

				if (numChannels > 0)
				{
					animation->channels = MAKE_SHASHMAP_IN_PLACE(r2::draw::AnimationChannel, startOfArrayPtr, hashMapSize);
					startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SHashMap<r2::draw::AnimationChannel>::MemorySize(hashMapSize));
				}

				animation->assetName = flatAnimationData->animationName();
				animation->ticksPerSeconds = flatAnimationData->ticksPerSeconds();

				double maxChannelDuration = 0;

				for (u32 i = 0; i < numChannels; ++i)
				{
					const flat::Channel* flatChannel = flatChannels->Get(i);

					r2::draw::AnimationChannel channel;

					channel.hashName = flatChannel->channelName();
					const auto positionKeys = flatChannel->positionKeys();
					const auto numPositionKeys = flatChannel->positionKeys()->size();
					if (numPositionKeys > 0)
					{
						channel.positionKeys = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::VectorKey, numPositionKeys);

						startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::VectorKey>::MemorySize(numPositionKeys));
					}

					for (u32 pKey = 0; pKey < numPositionKeys; ++pKey)
					{
						const auto flatPositionKey = positionKeys->Get(pKey);
						const auto flatPositionValue = flatPositionKey->value();

						r2::sarr::Push(*channel.positionKeys, { flatPositionKey->time(), glm::vec3(flatPositionValue.x(), flatPositionValue.y(), flatPositionValue.z()) });

						maxChannelDuration = std::max(flatPositionKey->time(), maxChannelDuration);
					}

					const auto scaleKeys = flatChannel->scaleKeys();
					const auto numScaleKeys = scaleKeys->size();

					if (numScaleKeys > 0)
					{
						channel.scaleKeys = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::VectorKey, numScaleKeys);
						startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::VectorKey>::MemorySize(numScaleKeys));
					}

					for (u32 sKey = 0; sKey < numScaleKeys; ++sKey)
					{
						const auto flatScaleKey = scaleKeys->Get(sKey);
						const auto flatScaleValue = flatScaleKey->value();
						r2::sarr::Push(*channel.scaleKeys, { flatScaleKey->time(), glm::vec3(flatScaleValue.x(), flatScaleValue.y(), flatScaleValue.z()) });

						maxChannelDuration = std::max(flatScaleKey->time(), maxChannelDuration);
					}
					const auto rotationKeys = flatChannel->rotationKeys();
					const auto numRotationKeys = rotationKeys->size();

					if (numRotationKeys > 0)
					{
						channel.rotationKeys = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::RotationKey, numRotationKeys);
						startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::RotationKey>::MemorySize(numRotationKeys));
					}

					for (u32 rKey = 0; rKey < numRotationKeys; ++rKey)
					{
						const auto flatRotationKey = rotationKeys->Get(rKey);
						const auto flatRotationValue = flatRotationKey->value();
						r2::sarr::Push(*channel.rotationKeys, { flatRotationKey->time(), glm::quat(flatRotationValue.w(), flatRotationValue.x(), flatRotationValue.y(), flatRotationValue.z()) });

						maxChannelDuration = std::max(flatRotationKey->time(), maxChannelDuration);
					}

					r2::shashmap::Set(*animation->channels, channel.hashName, channel);
				}

				//@TODO(Serge): this should be taken care of when we convert the animation, but for now this will suffice
				animation->duration = std::min(flatAnimationData->durationInTicks(), maxChannelDuration);


				r2::sarr::Push(*model->optrAnimations, animation);
			}
		}

		return true;
	}

}