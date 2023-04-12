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

	void SerializeSkeletalAnimationComponent(flexbuffers::Builder& builder, const SkeletalAnimationComponent& skeletalAnimationComponent)
	{
		builder.Vector("skeletalAnimationComponent", [&]() {
			builder.UInt(skeletalAnimationComponent.animModelAssetName);
			builder.UInt(skeletalAnimationComponent.shouldUseSameTransformsForAllInstances);
			builder.UInt(skeletalAnimationComponent.startTime);
			builder.UInt(skeletalAnimationComponent.shouldLoop);
			});
	}

	template<>
	inline void SerializeComponentArray(flexbuffers::Builder& builder, const r2::SArray<SkeletalAnimationComponent>& components)
	{
		builder.Vector("skeletalAnimationComponents", [&]() {

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
		builder.Vector("skeletalAnimationInstances", [&]() {
			const auto numComponents = r2::sarr::Size(components);
			for (u32 i = 0; i < numComponents; ++i)
			{
				const InstanceComponentT<SkeletalAnimationComponent>& instancedSkeletalAnimation = r2::sarr::At(components, i);

				for (u32 j = 0; j < instancedSkeletalAnimation.numInstances; ++j)
				{
					const auto& skeletalAnimationComponent = r2::sarr::At(*instancedSkeletalAnimation.instances, j);
					SerializeSkeletalAnimationComponent(builder, skeletalAnimationComponent);
				}
			}
		});
	}
}

#endif // __SKELETAL_ANIMATION_COMPONENT_SERIALIZATION_H__
