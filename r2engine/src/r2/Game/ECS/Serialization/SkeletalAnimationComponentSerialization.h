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

namespace r2::ecs
{
	/*
		struct SkeletalAnimationComponent
		{
			u64 animModelAssetName;

			b32 shouldUseSameTransformsForAllInstances;
			const r2::draw::AnimModel* animModel;
			u32 startTime;
			b32 shouldLoop;
			const r2::draw::Animation* animation;

			r2::SArray<r2::draw::ShaderBoneTransform>* shaderBones;
		};
	*/

	inline void SerializeSkeletalAnimationComponent(flatbuffers::FlatBufferBuilder& fbb,
		r2::SArray<flatbuffers::Offset<flat::SkeletalAnimationComponentData>>* skeletalAnimationComponents,
		const SkeletalAnimationComponent& skeletalAnimationComponent)
	{
		flat::SkeletalAnimationComponentDataBuilder skeletalAnimationComponentBuilder(fbb);

		skeletalAnimationComponentBuilder.add_animModelAssetName(skeletalAnimationComponent.animModelAssetName);
		skeletalAnimationComponentBuilder.add_shouldLoop(skeletalAnimationComponent.shouldLoop);
		skeletalAnimationComponentBuilder.add_shouldUseSameTransformForAllInstances(skeletalAnimationComponent.shouldUseSameTransformsForAllInstances);
		skeletalAnimationComponentBuilder.add_startingAnimationAssetName(skeletalAnimationComponent.startingAnimationAssetName);
		skeletalAnimationComponentBuilder.add_startTime(skeletalAnimationComponent.startTime);

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
		skeletalAnimationComponent.animModelAssetName = flatSkeletalAnimationComponent->animModelAssetName();
		skeletalAnimationComponent.startingAnimationAssetName = flatSkeletalAnimationComponent->startingAnimationAssetName();
		skeletalAnimationComponent.shouldUseSameTransformsForAllInstances = flatSkeletalAnimationComponent->shouldUseSameTransformForAllInstances();
		skeletalAnimationComponent.startTime = flatSkeletalAnimationComponent->startTime();
		skeletalAnimationComponent.shouldLoop = flatSkeletalAnimationComponent->shouldLoop();
	}

	template<>
	inline void DeSerializeComponentArray(r2::SArray<SkeletalAnimationComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		const flat::SkeletalAnimationComponentArrayData* skeletalAnimationComponentArrayData = flatbuffers::GetRoot<flat::SkeletalAnimationComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = skeletalAnimationComponentArrayData->skeletalAnimationComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::SkeletalAnimationComponentData* flatSkeletalAnimationComponent = componentVector->Get(i);

			SkeletalAnimationComponent skeletalAnimationComponent;
			skeletalAnimationComponent.animModelAssetName = 0;
			skeletalAnimationComponent.startingAnimationAssetName = 0;
			skeletalAnimationComponent.shouldUseSameTransformsForAllInstances = true;
			skeletalAnimationComponent.animModel = nullptr;
			skeletalAnimationComponent.shaderBones = nullptr;
			skeletalAnimationComponent.animation = nullptr;
			skeletalAnimationComponent.shouldLoop = false;
			skeletalAnimationComponent.startTime = 0;

			DeSerializeSkeletalAnimationComponent(skeletalAnimationComponent, flatSkeletalAnimationComponent);

			r2::sarr::Push(components, skeletalAnimationComponent);
		}
	}

	template<>
	inline void DeSerializeComponentArray(r2::SArray<InstanceComponentT<SkeletalAnimationComponent>>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		const flat::InstancedSkeletalAnimationComponentArrayData* instancedSkeletalAnimationComponentArrayData = flatbuffers::GetRoot<flat::InstancedSkeletalAnimationComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = instancedSkeletalAnimationComponentArrayData->instancedSkeletalAnimationComponentArray();

		for (size_t i = 0; i < componentVector->size(); ++i)
		{
			InstanceComponentT<SkeletalAnimationComponent> instancedSkeletalAnimationComponent;

			auto flatInstancedSkeletalAnimationComponent = componentVector->Get(i);

			size_t numInstances = flatInstancedSkeletalAnimationComponent->skeletalAnimationComponentArray()->size();

			instancedSkeletalAnimationComponent.numInstances = numInstances;

			//this is a problem right here - how do we free this?
			instancedSkeletalAnimationComponent.instances = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, SkeletalAnimationComponent, numInstances);

			for (size_t j = 0; j < numInstances; ++j)
			{
				SkeletalAnimationComponent skeletalAnimationComponent;
				skeletalAnimationComponent.animModelAssetName = 0;
				skeletalAnimationComponent.startingAnimationAssetName = 0;
				skeletalAnimationComponent.shouldUseSameTransformsForAllInstances = true;
				skeletalAnimationComponent.animModel = nullptr;
				skeletalAnimationComponent.shaderBones = nullptr;
				skeletalAnimationComponent.animation = nullptr;
				skeletalAnimationComponent.shouldLoop = false;
				skeletalAnimationComponent.startTime = 0;

				DeSerializeSkeletalAnimationComponent(skeletalAnimationComponent, flatInstancedSkeletalAnimationComponent->skeletalAnimationComponentArray()->Get(j));

				r2::sarr::Push(*instancedSkeletalAnimationComponent.instances, skeletalAnimationComponent);
			}

			r2::sarr::Push(components, instancedSkeletalAnimationComponent);
		}
	}

	template<>
	inline void CleanupDeserializeComponentArray(r2::SArray<InstanceComponentT<SkeletalAnimationComponent>>& components)
	{
		s32 size = r2::sarr::Size(components);

		for (s32 i = size - 1; i >= 0; --i)
		{
			const InstanceComponentT<SkeletalAnimationComponent>& instancedSkeletalAnimationComponent = r2::sarr::At(components, i);
			FREE(instancedSkeletalAnimationComponent.instances, *MEM_ENG_SCRATCH_PTR);
		}
	}


}

#endif // __SKELETAL_ANIMATION_COMPONENT_SERIALIZATION_H__