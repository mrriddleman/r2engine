#ifndef __AUDIO_EMITTER_COMPONENT_SERIALIZATION_H__
#define __AUDIO_EMITTER_COMPONENT_SERIALIZATION_H__

#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"

#include "r2/Game/ECS/Components/AudioEmitterComponent.h"
#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"
#include "r2/Game/ECS/Serialization/AudioEmitterComponentArrayData_generated.h"
#include "r2/Utils/Utils.h"

namespace r2::ecs
{
	template<>
	inline void SerializeComponentArray(flatbuffers::FlatBufferBuilder& fbb, const r2::SArray<AudioEmitterComponent>& components)
	{
		const auto numComponents = r2::sarr::Size(components);

		r2::SArray<flatbuffers::Offset<flat::AudioEmitterComponentData>>* flatAudioEmitterComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::AudioEmitterComponentData>, numComponents);

		for (u32 i = 0; i < numComponents; ++i)
		{
			const auto& audioEmitterComponent = r2::sarr::At(components, i);

			r2::SArray<flatbuffers::Offset<flat::AudioEmitterParameter>>* flatAudioEmitterParameters = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, flatbuffers::Offset<flat::AudioEmitterParameter>, audioEmitterComponent.numParameters);

			for (u32 p = 0; p < audioEmitterComponent.numParameters; ++p)
			{
				auto paramName = fbb.CreateString(audioEmitterComponent.parameters[p].parameterName);
				flat::AudioEmitterParameterBuilder paramBuilder(fbb);
				paramBuilder.add_parameterName(paramName);
				paramBuilder.add_value(audioEmitterComponent.parameters[p].parameterValue);

				r2::sarr::Push(*flatAudioEmitterParameters, paramBuilder.Finish());
			}

			auto flatEventName = fbb.CreateString(audioEmitterComponent.eventName);
			auto flatParamsVec = fbb.CreateVector(flatAudioEmitterParameters->mData, flatAudioEmitterParameters->mSize);
			flat::AudioEmitterComponentDataBuilder audioEmitterComponentBuilder(fbb);
			audioEmitterComponentBuilder.add_eventName(flatEventName);
			audioEmitterComponentBuilder.add_startCondition(static_cast<flat::AudioEmitterStartCondition>(audioEmitterComponent.startCondition));
			audioEmitterComponentBuilder.add_parameters(flatParamsVec);
			audioEmitterComponentBuilder.add_allowFadeoutWhenStopping(audioEmitterComponent.allowFadeoutWhenStopping > 0);
			audioEmitterComponentBuilder.add_releaseAfterPlay(audioEmitterComponent.releaseAfterPlay > 0);

			r2::sarr::Push(*flatAudioEmitterComponents, audioEmitterComponentBuilder.Finish());

			FREE(flatAudioEmitterParameters, *MEM_ENG_SCRATCH_PTR);
		}

		auto vec = fbb.CreateVector(flatAudioEmitterComponents->mData, flatAudioEmitterComponents->mSize);

		flat::AudioEmitterComponentArrayDataBuilder audioEmitterComponentArrayDataBuilder(fbb);

		audioEmitterComponentArrayDataBuilder.add_audioEmitterComponentArray(vec);

		fbb.Finish(audioEmitterComponentArrayDataBuilder.Finish());

		FREE(flatAudioEmitterComponents, *MEM_ENG_SCRATCH_PTR);
	}

	template<>
	inline void DeSerializeComponentArray(r2::SArray<AudioEmitterComponent>& components, const r2::SArray<Entity>* entities, const r2::SArray<const flat::EntityData*>* refEntities, const flat::ComponentArrayData* componentArrayData)
	{
		R2_CHECK(r2::sarr::Size(components) == 0, "Shouldn't have anything in there yet?");
		R2_CHECK(componentArrayData != nullptr, "Shouldn't be nullptr");

		const flat::AudioEmitterComponentArrayData* audioEmitterComponentArrayData = flatbuffers::GetRoot<flat::AudioEmitterComponentArrayData>(componentArrayData->componentArray()->data());

		const auto* componentVector = audioEmitterComponentArrayData->audioEmitterComponentArray();

		for (flatbuffers::uoffset_t i = 0; i < componentVector->size(); ++i)
		{
			const flat::AudioEmitterComponentData* flatAudioEmitterComponent = componentVector->Get(i);

			AudioEmitterComponent newAudioEmitterComponent;
			r2::util::PathCpy(newAudioEmitterComponent.eventName, flatAudioEmitterComponent->eventName()->c_str());
			newAudioEmitterComponent.numParameters = flatAudioEmitterComponent->parameters()->size();
			newAudioEmitterComponent.startCondition = static_cast<AudioEmitterStartCondition>(flatAudioEmitterComponent->startCondition());
			newAudioEmitterComponent.allowFadeoutWhenStopping = flatAudioEmitterComponent->allowFadeoutWhenStopping();
			newAudioEmitterComponent.releaseAfterPlay = flatAudioEmitterComponent->releaseAfterPlay();
			newAudioEmitterComponent.eventInstanceHandle = r2::audio::AudioEngine::InvalidEventInstanceHandle;

			for (u32 p = 0; p < newAudioEmitterComponent.numParameters; ++p)
			{
				r2::util::PathCpy(newAudioEmitterComponent.parameters[p].parameterName, flatAudioEmitterComponent->parameters()->Get(p)->parameterName()->c_str());
				newAudioEmitterComponent.parameters[p].parameterValue = flatAudioEmitterComponent->parameters()->Get(p)->value();
			}

			r2::sarr::Push(components, newAudioEmitterComponent);
		}
	}

}

#endif