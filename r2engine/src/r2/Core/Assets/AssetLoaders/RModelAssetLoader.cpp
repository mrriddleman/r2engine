#include "r2pch.h"

#include "r2/Core/Assets/AssetLoaders/RModelAssetLoader.h"
#include "assetlib/RModel_generated.h"
#include "assetlib/RModelMetaData_generated.h"
#include "assetlib/ModelAsset.h"
#include "r2/Core/Assets/AssetFiles/MemoryAssetFile.h"
#include "r2/Render/Model/Model.h"
#include <glm/glm.hpp>

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
		
		if (metaData->isAnimatedModel())
		{
			totalSize += r2::draw::Skeleton::MemorySizeNoData(metaData->skeletonMetaData()->numJoints(), alignment, header, boundsChecking);
			
			const auto numBones = metaData->boneMetaData()->numBoneInfo();

			u64 hashCapacity = static_cast<u64>(std::round(numBones * r2::SHashMap<u32>::LoadFactorMultiplier()));

			totalSize += r2::draw::AnimModel::MemorySizeNoData(hashCapacity, totalVertices, numBones, metaData->numMeshes(), metaData->numMaterials(), alignment, header, boundsChecking);

			return static_cast<u64>(totalSize);
		}
		
		totalSize += r2::draw::Model::ModelMemorySize(metaData->numMeshes(), metaData->numMaterials(), alignment, header, boundsChecking);

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

		if (metaData->isAnimatedModel())
		{
			const auto numBones = metaData->boneMetaData()->numBoneInfo();

			r2::draw::AnimModel* model = new (dataPtr) r2::draw::AnimModel();

			startOfArrayPtr = r2::mem::utils::PointerAdd(dataPtr, sizeof(r2::draw::AnimModel));

			model->model.optrMaterialHandles = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::MaterialHandle, numMaterials);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::MaterialHandle>::MemorySize(numMaterials));

			model->model.optrMeshes = EMPLACE_SARRAY(startOfArrayPtr, const r2::draw::Mesh*, numMeshes);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<const r2::draw::Mesh*>::MemorySize(numMeshes));

			model->boneData = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::BoneData, numVertices);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::BoneData>::MemorySize(numVertices));

			model->boneInfo = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::BoneInfo, numBones);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::BoneInfo>::MemorySize(numBones));

			u64 hashCapacity = static_cast<u64>(std::round(numBones * r2::SHashMap<u32>::LoadFactorMultiplier()));

			model->boneMapping = MAKE_SHASHMAP_IN_PLACE(s32, startOfArrayPtr, hashCapacity);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SHashMap<u32>::MemorySize(hashCapacity));


			//Process the Nodes

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

			

			//Model data
			const flat::Matrix4* flatGlobalInverse = modelData->globalInverseTransform();

			model->model.globalInverseTransform = GetGLMMatrix4FromFlatMatrix(flatGlobalInverse);

			//materials

			const auto flatMaterialNames = modelData->materials();

			for (flatbuffers::uoffset_t i = 0; i < flatMaterialNames->size(); ++i)
			{
				//@TODO(Serge): get rid of these material handles
				r2::draw::MaterialHandle materialHandle = r2::draw::matsys::FindMaterialHandle(flatMaterialNames->Get(i)->name());

				R2_CHECK(!r2::draw::mat::IsInvalidHandle(materialHandle), "We should have the material handle!");

				r2::sarr::Push(*model->model.optrMaterialHandles, materialHandle);
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

				nextMeshPtr->hashName = metaData->meshInfos()->Get(i)->meshName();
				nextMeshPtr->materialIndex = modelData->meshes()->Get(i)->materialIndex();

				r2::sarr::Push(*model->model.optrMeshes, const_cast<const r2::draw::Mesh*>(nextMeshPtr));
			}

			//Animation data
			const auto flatBoneInfos = modelData->animationData()->boneInfo();

			for (flatbuffers::uoffset_t i = 0; i < flatBoneInfos->size(); ++i)
			{
				r2::draw::BoneInfo boneInfo;
				boneInfo.offsetTransform = GetGLMMatrix4FromFlatMatrix(&flatBoneInfos->Get(i)->offsetTransform());

				r2::sarr::Push(*model->boneInfo, boneInfo);
			}

			const auto flatBoneData = modelData->animationData()->boneData();

			for (flatbuffers::uoffset_t i = 0; i < flatBoneData->size(); ++i)
			{
				r2::draw::BoneData boneData;
				boneData.boneWeights = GetVec4FromFlatVec4(&flatBoneData->Get(i)->boneWeights());
				boneData.boneIDs = GetIVec4FromFlatIVec4(&flatBoneData->Get(i)->boneIDs());

				r2::sarr::Push(*model->boneData, boneData);
			}

			const auto flatBoneMapEntries = modelData->animationData()->boneMapping();

			for (flatbuffers::uoffset_t i = 0; i < flatBoneMapEntries->size(); ++i)
			{
				r2::shashmap::Set(*model->boneMapping, flatBoneMapEntries->Get(i)->key(), flatBoneMapEntries->Get(i)->val());
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
		else
		{
			r2::draw::Model* model = new (dataPtr) r2::draw::Model();

			R2_CHECK(model != nullptr, "We should have a proper model!");

			startOfArrayPtr = r2::mem::utils::PointerAdd(dataPtr, sizeof(r2::draw::Model));

			model->optrMaterialHandles = EMPLACE_SARRAY(startOfArrayPtr, r2::draw::MaterialHandle, numMaterials);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::MaterialHandle>::MemorySize(numMaterials));

			model->optrMeshes = EMPLACE_SARRAY(startOfArrayPtr, const r2::draw::Mesh*, numMeshes);

			startOfArrayPtr = r2::mem::utils::PointerAdd(startOfArrayPtr, r2::SArray<r2::draw::Mesh>::MemorySize(numMeshes));

			const flat::Matrix4* flatGlobalInverse = modelData->globalInverseTransform();

			model->globalInverseTransform = GetGLMMatrix4FromFlatMatrix(flatGlobalInverse);

			//materials

			const auto flatMaterialNames = modelData->materials();

			for (flatbuffers::uoffset_t i = 0; i < flatMaterialNames->size(); ++i)
			{
				r2::draw::MaterialHandle materialHandle = r2::draw::matsys::FindMaterialHandle(flatMaterialNames->Get(i)->name());

				R2_CHECK(!r2::draw::mat::IsInvalidHandle(materialHandle), "We should have the material handle!");

				r2::sarr::Push(*model->optrMaterialHandles, materialHandle);
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

				nextMeshPtr->hashName = metaData->meshInfos()->Get(i)->meshName();
				nextMeshPtr->materialIndex = modelData->meshes()->Get(i)->materialIndex();
				const auto bounds = metaData->meshInfos()->Get(i)->meshBounds();

				nextMeshPtr->objectBounds.radius = bounds->radius();
				nextMeshPtr->objectBounds.origin = GetVec3FromFlatVec3(bounds->origin());
				nextMeshPtr->objectBounds.extents = GetVec3FromFlatVec3(bounds->extents());

				r2::sarr::Push(*model->optrMeshes, const_cast<const r2::draw::Mesh*>(nextMeshPtr));
			}
		}


		return true;
	}

}