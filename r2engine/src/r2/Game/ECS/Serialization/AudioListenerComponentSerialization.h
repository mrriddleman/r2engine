#ifndef __AUDIO_LISTENER_COMPONENT_SERIALIZATION_H__
#define __AUDIO_LISTENER_COMPONENT_SERIALIZATION_H__

#include "r2/Game/ECS/Components/AudioListenerComponent.h"
#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Serialization/AudioListenerComponentArrayData_generated.h"

#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"

namespace r2::ecs
{
	template<>
	inline void SerializeComponentArray(flatbuffers::FlatBufferBuilder& fbb, const r2::SArray<AudioListenerComponent>& components)
	{
		const auto numComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::AudioListenerComponentData>>* audioListenerComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::AudioListenerComponentData>, numComponents);

		for (u32 i = 0; i < numComponents; ++i)
		{
			const AudioListenerComponent& audioListenerComponent = r2::sarr::At(components, i);

			flat::AudioListenerBuilder audioListenerBuilder(fbb);
			audioListenerBuilder.add_listener(audioListenerComponent.listener);

			auto flatAudioListener = audioListenerBuilder.Finish();

			flat::AudioListenerComponentDataBuilder audioListenerComponentBuilder(fbb);
			audioListenerComponentBuilder.add_listener(flatAudioListener);
			audioListenerComponentBuilder.add_entityToFollow(audioListenerComponent.entityToFollow);
			
			r2::sarr::Push(*audioListenerComponents, audioListenerComponentBuilder.Finish());
		}

		auto vec = fbb.CreateVector(audioListenerComponents->mData, audioListenerComponents->mSize);

		flat::AudioListenerComponentArrayDataBuilder audioListenerComponentArrayDataBuilder(fbb);

		audioListenerComponentArrayDataBuilder.add_audioListenerComponentArray(vec);

		fbb.Finish(audioListenerComponentArrayDataBuilder.Finish());

		FREE(audioListenerComponents, *MEM_ENG_SCRATCH_PTR);
	}

	template<>
	inline void DeSerializeComponentArray(ECSWorld& ecsWorld, r2::SArray<AudioListenerComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		R2_CHECK(r2::sarr::Size(components) == 0, "Shouldn't have anything in there yet?");
		R2_CHECK(componentArrayData != nullptr, "Shouldn't be nullptr");

		const flat::AudioListenerComponentArrayData* audioListenerComponentArrayData = flatbuffers::GetRoot<flat::AudioListenerComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = audioListenerComponentArrayData->audioListenerComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::AudioListenerComponentData* flatAudioListenerComponent = componentVector->Get(i);

			Entity entityToFollow = MapSerializedEntity(flatAudioListenerComponent->entityToFollow(), entities, refEntities);

			AudioListenerComponent newAudioListenerComponent;
			newAudioListenerComponent.entityToFollow = entityToFollow;
			newAudioListenerComponent.listener = flatAudioListenerComponent->listener()->listener();

			r2::sarr::Push(components, newAudioListenerComponent);
		}
	}
}

#endif
