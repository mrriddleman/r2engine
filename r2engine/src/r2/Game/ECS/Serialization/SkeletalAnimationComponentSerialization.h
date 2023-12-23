#ifndef __SKELETAL_ANIMATION_COMPONENT_SERIALIZATION_H__
#define __SKELETAL_ANIMATION_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Serialization/SkeletalAnimationComponentArrayData_generated.h"
#include "r2/Game/ECS/Serialization/InstancedSkeletalAnimationComponentArrayData_generated.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Engine.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"

namespace r2::ecs
{
	/*
		struct SkeletalAnimationComponent
		{
			u64 animModelAssetName;
			u32 startingAnimationIndex;
			u32 startTime;
			b32 shouldLoop;
			b32 shouldUseSameTransformsForAllInstances;

			//These shouldn't be edited directly in the editor just set when the above is set
			u32 currentAnimationIndex;
			const r2::draw::Model* animModel;
			r2::SArray<r2::draw::ShaderBoneTransform>* shaderBones;
		};
	*/

	inline void SerializeSkeletalAnimationComponent(flatbuffers::FlatBufferBuilder& fbb,
		r2::SArray<flatbuffers::Offset<flat::SkeletalAnimationComponentData>>* skeletalAnimationComponents,
		const SkeletalAnimationComponent& skeletalAnimationComponent)
	{
#ifdef R2_ASSET_PIPELINE
		auto flatAssetName = flat::CreateAssetName(fbb, 0, skeletalAnimationComponent.animModelAssetName.hashID, fbb.CreateString(skeletalAnimationComponent.animModelAssetName.assetNameString));
#else
		auto flatAssetName = flat::CreateAssetName(fbb, 0, skeletalAnimationComponent.animModelAssetName.hashID, fbb.CreateString(""));
#endif
		

		flat::SkeletalAnimationComponentDataBuilder skeletalAnimationComponentBuilder(fbb);

		skeletalAnimationComponentBuilder.add_animModelAssetName(flatAssetName);
		skeletalAnimationComponentBuilder.add_startingAnimationIndex(skeletalAnimationComponent.startingAnimationIndex);
		skeletalAnimationComponentBuilder.add_startTime(skeletalAnimationComponent.startTime);
		skeletalAnimationComponentBuilder.add_shouldLoop(skeletalAnimationComponent.shouldLoop);
		skeletalAnimationComponentBuilder.add_shouldUseSameTransformForAllInstances(skeletalAnimationComponent.shouldUseSameTransformsForAllInstances);

		r2::sarr::Push(*skeletalAnimationComponents, skeletalAnimationComponentBuilder.Finish());
	}

	template<>
	inline void SerializeComponentArray(flatbuffers::FlatBufferBuilder& fbb, const r2::SArray<SkeletalAnimationComponent>& components)
	{
		const auto numComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::SkeletalAnimationComponentData>>* skelatalAnimationComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::SkeletalAnimationComponentData>, numComponents);

		for (u32 i = 0; i < numComponents; ++i)
		{
			const SkeletalAnimationComponent& skeletalAnimationComponent = r2::sarr::At(components, i);

			SerializeSkeletalAnimationComponent(fbb, skelatalAnimationComponents, skeletalAnimationComponent);
		}

		auto vec = fbb.CreateVector(skelatalAnimationComponents->mData, skelatalAnimationComponents->mSize);

		flat::SkeletalAnimationComponentArrayDataBuilder skeletalAnimationComponentArrayDataBuilder(fbb);

		skeletalAnimationComponentArrayDataBuilder.add_skeletalAnimationComponentArray(vec);

		fbb.Finish(skeletalAnimationComponentArrayDataBuilder.Finish());

		FREE(skelatalAnimationComponents, *MEM_ENG_SCRATCH_PTR);
	}

	template<>
	inline void SerializeComponentArray(flatbuffers::FlatBufferBuilder& fbb, const r2::SArray<InstanceComponentT<SkeletalAnimationComponent>>& components)
	{
		const auto numComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::SkeletalAnimationComponentArrayData>>* instancedSkeletalAnimationComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::SkeletalAnimationComponentArrayData>, numComponents);

		for (u32 i = 0; i < numComponents; ++i)
		{
			const InstanceComponentT<SkeletalAnimationComponent>& instancedSkeletalAnimationComponent = r2::sarr::At(components, i);

			r2::SArray< flatbuffers::Offset<flat::SkeletalAnimationComponentData>>* skeletalAnimationComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::SkeletalAnimationComponentData>, instancedSkeletalAnimationComponent.numInstances);

			for (u32 j = 0; j < instancedSkeletalAnimationComponent.numInstances; ++j)
			{
				const SkeletalAnimationComponent& skeletalAnimationComponent = r2::sarr::At(*instancedSkeletalAnimationComponent.instances, j);

				SerializeSkeletalAnimationComponent(fbb, skeletalAnimationComponents, skeletalAnimationComponent);
			}

			auto vec = fbb.CreateVector(skeletalAnimationComponents->mData, skeletalAnimationComponents->mSize);

			flat::SkeletalAnimationComponentArrayDataBuilder skeletalAnimationComponentArrayDataBuilder(fbb);

			skeletalAnimationComponentArrayDataBuilder.add_skeletalAnimationComponentArray(vec);

			r2::sarr::Push(*instancedSkeletalAnimationComponents, skeletalAnimationComponentArrayDataBuilder.Finish());

			FREE(skeletalAnimationComponents, *MEM_ENG_SCRATCH_PTR);
		}

		auto vec = fbb.CreateVector(instancedSkeletalAnimationComponents->mData, instancedSkeletalAnimationComponents->mSize);

		flat::InstancedSkeletalAnimationComponentArrayDataBuilder instancedSkeletalAnimationComponentArrayDataBuilder(fbb);

		instancedSkeletalAnimationComponentArrayDataBuilder.add_instancedSkeletalAnimationComponentArray(vec);

		fbb.Finish(instancedSkeletalAnimationComponentArrayDataBuilder.Finish());

		FREE(instancedSkeletalAnimationComponents, *MEM_ENG_SCRATCH_PTR);
	}

	inline void DeSerializeSkeletalAnimationComponent(SkeletalAnimationComponent& skeletalAnimationComponent, const flat::SkeletalAnimationComponentData* flatSkeletalAnimationComponent)
	{
		r2::asset::MakeAssetNameFromFlatAssetName(flatSkeletalAnimationComponent->animModelAssetName(), skeletalAnimationComponent.animModelAssetName);
		skeletalAnimationComponent.startingAnimationIndex = flatSkeletalAnimationComponent->startingAnimationIndex();
		skeletalAnimationComponent.shouldUseSameTransformsForAllInstances = flatSkeletalAnimationComponent->shouldUseSameTransformForAllInstances();
		skeletalAnimationComponent.startTime = flatSkeletalAnimationComponent->startTime();
		skeletalAnimationComponent.shouldLoop = flatSkeletalAnimationComponent->shouldLoop();
	}

	template<>
	inline void DeSerializeComponentArray(ECSWorld& ecsWorld, r2::SArray<SkeletalAnimationComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		const flat::SkeletalAnimationComponentArrayData* skeletalAnimationComponentArrayData = flatbuffers::GetRoot<flat::SkeletalAnimationComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = skeletalAnimationComponentArrayData->skeletalAnimationComponentArray();
		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::SkeletalAnimationComponentData* flatSkeletalAnimationComponent = componentVector->Get(i);

			SkeletalAnimationComponent skeletalAnimationComponent;
			skeletalAnimationComponent.animModelAssetName.hashID = 0;
#ifdef R2_ASSET_PIPELINE
			skeletalAnimationComponent.animModelAssetName.assetNameString= "";
#endif
			skeletalAnimationComponent.startingAnimationIndex = 0;
			skeletalAnimationComponent.shouldUseSameTransformsForAllInstances = true;
			skeletalAnimationComponent.animModel = nullptr;
			skeletalAnimationComponent.shaderBones = nullptr;
			skeletalAnimationComponent.currentAnimationIndex = skeletalAnimationComponent.startingAnimationIndex;
			skeletalAnimationComponent.shouldLoop = false;
			skeletalAnimationComponent.startTime = 0;

			DeSerializeSkeletalAnimationComponent(skeletalAnimationComponent, flatSkeletalAnimationComponent);

			r2::asset::Asset modelAsset = r2::asset::Asset(skeletalAnimationComponent.animModelAssetName, r2::asset::RMODEL);
			r2::draw::ModelHandle modelHandle = gameAssetManager.LoadAsset(modelAsset);
			skeletalAnimationComponent.animModel = gameAssetManager.GetAssetDataConst<r2::draw::Model>(modelHandle);

			skeletalAnimationComponent.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, r2::sarr::Size(*skeletalAnimationComponent.animModel->optrBoneInfo));

			r2::sarr::Clear(*skeletalAnimationComponent.shaderBones);

			r2::sarr::Push(components, skeletalAnimationComponent);
		}
	}

	template<>
	inline void DeSerializeComponentArray(ECSWorld& ecsWorld, r2::SArray<InstanceComponentT<SkeletalAnimationComponent>>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		const flat::InstancedSkeletalAnimationComponentArrayData* instancedSkeletalAnimationComponentArrayData = flatbuffers::GetRoot<flat::InstancedSkeletalAnimationComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = instancedSkeletalAnimationComponentArrayData->instancedSkeletalAnimationComponentArray();
		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		for (size_t i = 0; i < componentVector->size(); ++i)
		{
			InstanceComponentT<SkeletalAnimationComponent> instancedSkeletalAnimationComponent;

			auto flatInstancedSkeletalAnimationComponent = componentVector->Get(i);

			size_t numInstances = flatInstancedSkeletalAnimationComponent->skeletalAnimationComponentArray()->size();

			instancedSkeletalAnimationComponent.numInstances = numInstances;

			//this is a problem right here - how do we free this?
			instancedSkeletalAnimationComponent.instances = ECS_WORLD_MAKE_SARRAY(ecsWorld, SkeletalAnimationComponent, numInstances);

			for (size_t j = 0; j < numInstances; ++j)
			{
				SkeletalAnimationComponent skeletalAnimationComponent;
				skeletalAnimationComponent.animModelAssetName.hashID = 0;
#ifdef R2_ASSET_PIPELINE
				skeletalAnimationComponent.animModelAssetName.assetNameString = "";
#endif
				skeletalAnimationComponent.startingAnimationIndex = 0;
				skeletalAnimationComponent.shouldUseSameTransformsForAllInstances = true;
				skeletalAnimationComponent.animModel = nullptr;
				skeletalAnimationComponent.shaderBones = nullptr;
				skeletalAnimationComponent.currentAnimationIndex = skeletalAnimationComponent.startingAnimationIndex;
				skeletalAnimationComponent.shouldLoop = false;
				skeletalAnimationComponent.startTime = 0;

				DeSerializeSkeletalAnimationComponent(skeletalAnimationComponent, flatInstancedSkeletalAnimationComponent->skeletalAnimationComponentArray()->Get(j));

				r2::asset::Asset modelAsset = r2::asset::Asset(skeletalAnimationComponent.animModelAssetName, r2::asset::RMODEL);
				r2::draw::ModelHandle modelHandle = gameAssetManager.LoadAsset(modelAsset);
				skeletalAnimationComponent.animModel = gameAssetManager.GetAssetDataConst<r2::draw::Model>(modelHandle);

				skeletalAnimationComponent.shaderBones = ECS_WORLD_MAKE_SARRAY(ecsWorld, r2::draw::ShaderBoneTransform, r2::sarr::Size(*skeletalAnimationComponent.animModel->optrBoneInfo));
				
				skeletalAnimationComponent.currentAnimationIndex = skeletalAnimationComponent.startingAnimationIndex;

				r2::sarr::Clear(*skeletalAnimationComponent.shaderBones);

				r2::sarr::Push(*instancedSkeletalAnimationComponent.instances, skeletalAnimationComponent);
			}

			r2::sarr::Push(components, instancedSkeletalAnimationComponent);
		}
	}

}

#endif // __SKELETAL_ANIMATION_COMPONENT_SERIALIZATION_H__
