#ifndef __SKELETAL_ANIMATION_COMPONENT_SERIALIZATION_H__
#define __SKELETAL_ANIMATION_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"

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

	inline void SerializeSkeletalAnimationComponent(flexbuffers::Builder& builder, const SkeletalAnimationComponent& skeletalAnimationComponent)
	{
		builder.Vector([&]() {
			builder.UInt(skeletalAnimationComponent.animModelAssetName);
			builder.UInt(skeletalAnimationComponent.startingAnimationAssetName);
			builder.UInt(skeletalAnimationComponent.shouldUseSameTransformsForAllInstances);
			builder.UInt(skeletalAnimationComponent.startTime);
			builder.UInt(skeletalAnimationComponent.shouldLoop);
			});
	}

	template<>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<SkeletalAnimationComponent>& components)
	{
		builder.Vector([&]() {

			const auto numComponents = r2::sarr::Size(components);
			for (u32 i = 0; i < numComponents; ++i)
			{
				const auto& skeletalAnimationComponent = r2::sarr::At(components, i);
				SerializeSkeletalAnimationComponent(builder, skeletalAnimationComponent);
			}
		});
	}

	template<>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<InstanceComponentT<SkeletalAnimationComponent>>& components)
	{
		builder.Vector([&]() {
			const auto numComponents = r2::sarr::Size(components);
			for (u32 i = 0; i < numComponents; ++i)
			{
				const InstanceComponentT<SkeletalAnimationComponent>& instancedSkeletalAnimation = r2::sarr::At(components, i);

				builder.Vector([&]() {
					for (u32 j = 0; j < instancedSkeletalAnimation.numInstances; ++j)
					{
						const auto& skeletalAnimationComponent = r2::sarr::At(*instancedSkeletalAnimation.instances, j);
						SerializeSkeletalAnimationComponent(builder, skeletalAnimationComponent);
					}
				});
			}
		});
	}

	inline void DeSerializeSkeletalAnimationComponent(SkeletalAnimationComponent& skeletalAnimationComponent, flexbuffers::Reference flexSkeletalAnimationRef)
	{
		auto flexSkeletalAnimationVector = flexSkeletalAnimationRef.AsVector();

		skeletalAnimationComponent.animModelAssetName = flexSkeletalAnimationVector[0].AsUInt64();
		skeletalAnimationComponent.startingAnimationAssetName = flexSkeletalAnimationVector[1].AsUInt64();
		skeletalAnimationComponent.shouldUseSameTransformsForAllInstances = flexSkeletalAnimationVector[2].AsBool();
		skeletalAnimationComponent.startTime = flexSkeletalAnimationVector[3].AsUInt32();
		skeletalAnimationComponent.shouldLoop = flexSkeletalAnimationVector[4].AsBool();
	}

	template<>
	inline void DeSerializeComponentArray(r2::SArray<SkeletalAnimationComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		auto componentVector = componentArrayData->componentArray_flexbuffer_root().AsVector();

		for (size_t i = 0; i < componentVector.size(); ++i)
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

			DeSerializeSkeletalAnimationComponent(skeletalAnimationComponent, componentVector[i]);

			r2::sarr::Push(components, skeletalAnimationComponent);
		}
	}

	template<>
	inline void DeSerializeComponentArray(r2::SArray<InstanceComponentT<SkeletalAnimationComponent>>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		auto componentVector = componentArrayData->componentArray_flexbuffer_root().AsVector();

		for (size_t i = 0; i < componentVector.size(); ++i)
		{
			InstanceComponentT<SkeletalAnimationComponent> instancedSkeletalAnimationComponent;

			auto flexInstancedSkeletalAnimationComponent = componentVector[i].AsVector();

			size_t numInstances = flexInstancedSkeletalAnimationComponent.size();

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

				DeSerializeSkeletalAnimationComponent(skeletalAnimationComponent, flexInstancedSkeletalAnimationComponent[j]);

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
