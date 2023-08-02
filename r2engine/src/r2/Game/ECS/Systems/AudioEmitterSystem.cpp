#include "r2pch.h"

#include "r2/Game/ECS/Systems/AudioEmitterSystem.h"
#include "r2/Game/ECS/Components/AudioEmitterComponent.h"
#include "r2/Game/ECS/Components/AudioEmitterActionComponent.h"
#include "r2/Game/ECS/Components/AudioParameterComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"

namespace r2::ecs
{

	AudioEmitterSystem::AudioEmitterSystem()
	{
		mKeepSorted = false;
	}

	AudioEmitterSystem::~AudioEmitterSystem()
	{

	}

	void AudioEmitterSystem::Update()
	{
		R2_CHECK(mnoptrCoordinator != nullptr, "Not sure why this should ever be nullptr?");
		R2_CHECK(mEntities != nullptr, "Not sure why this should ever be nullptr?");

		r2::audio::AudioEngine audioEngine;

		const auto numEntities = r2::sarr::Size(*mEntities);

		for (u32 i = 0; i < numEntities; ++i)
		{
			Entity e = r2::sarr::At(*mEntities, i);

			AudioEmitterComponent& audioEmitterComponent = mnoptrCoordinator->GetComponent<AudioEmitterComponent>(e);
			const TransformComponent* transformComponent = mnoptrCoordinator->GetComponentPtr<TransformComponent>(e);
			const AudioEmitterActionComponent* emitterActionComponent = mnoptrCoordinator->GetComponentPtr<AudioEmitterActionComponent>(e);
			const AudioParameterComponent* parameterComponent = mnoptrCoordinator->GetComponentPtr<AudioParameterComponent>(e);

			bool attributesValid = false;
			r2::audio::AudioEngine::Attributes3D attributes;
			if (transformComponent)
			{
				attributesValid = true;
				attributes.up = transformComponent->accumTransform.rotation * glm::vec3(0, 0, 1);
				attributes.look = transformComponent->accumTransform.rotation * glm::vec3(0, 1, 0);
				attributes.position = transformComponent->accumTransform.position;
				attributes.velocity = glm::vec3(0);
			}

			if (emitterActionComponent)
			{
				switch (emitterActionComponent->action)
				{
				case AEA_CREATE:
				{
					if (audioEngine.IsEventInstanceHandleValid(audioEmitterComponent.eventInstanceHandle))
					{
						audioEngine.ReleaseEventInstance(audioEmitterComponent.eventInstanceHandle);
					}

					audioEmitterComponent.eventInstanceHandle = audioEngine.CreateEventInstance(audioEmitterComponent.eventName);

					//set all of the initial parameters that are associated with this emitter
					for (u32 p = 0; p < audioEmitterComponent.numParameters; ++p)
					{
						audioEngine.SetEventParameterByName(
							audioEmitterComponent.eventInstanceHandle,
							audioEmitterComponent.parameters[p].parameterName,
							audioEmitterComponent.parameters[p].parameterValue);
					}

					if (audioEmitterComponent.startCondition == PLAY_ON_CREATE)
					{
						PlayEvent(audioEmitterComponent, attributes, attributesValid);
					}

					break;
				}
				case AEA_PLAY:
				{
					PlayEvent(audioEmitterComponent, attributes, attributesValid);
					break;
				}
				case AEA_STOP:
				{
					if (audioEngine.IsEventInstanceHandleValid(audioEmitterComponent.eventInstanceHandle))
					{
						audioEngine.StopEvent(audioEmitterComponent.eventInstanceHandle, audioEmitterComponent.allowFadeoutWhenStopping);
					}

					break;
				}
				case AEA_PAUSE:
				{
					if (audioEngine.IsEventInstanceHandleValid(audioEmitterComponent.eventInstanceHandle))
					{
						audioEngine.PauseEvent(audioEmitterComponent.eventInstanceHandle);
					}
					break;
				}
				case AEA_DESTROY:
				{
					if (audioEngine.IsEventInstanceHandleValid(audioEmitterComponent.eventInstanceHandle))
					{
						audioEngine.ReleaseEventInstance(audioEmitterComponent.eventInstanceHandle);
						audioEmitterComponent.eventInstanceHandle = r2::audio::AudioEngine::InvalidEventInstanceHandle;
					}
					break;
				}
				}

				mnoptrCoordinator->RemoveComponent<AudioEmitterActionComponent>(e);
			}

			if (audioEngine.IsEventInstanceHandleValid(audioEmitterComponent.eventInstanceHandle))
			{
				if (attributesValid)
				{
					audioEngine.SetAttributes3DForEvent(audioEmitterComponent.eventInstanceHandle, attributes);
				}
				
				if (parameterComponent)
				{
					audioEngine.SetEventParameterByName(audioEmitterComponent.eventInstanceHandle, parameterComponent->parameterName, parameterComponent->parameterValue);

					mnoptrCoordinator->RemoveComponent<AudioParameterComponent>(e);
				}
			}
		}
	}


	void AudioEmitterSystem::PlayEvent(AudioEmitterComponent& audioEmitterComponent, const r2::audio::AudioEngine::Attributes3D& attributes, bool attributesValid)
	{
		r2::audio::AudioEngine audioEngine;

		if (audioEngine.IsEventInstanceHandleValid(audioEmitterComponent.eventInstanceHandle))
		{
			audioEngine.PlayEvent(audioEmitterComponent.eventInstanceHandle, audioEmitterComponent.releaseAfterPlay);

			if (audioEmitterComponent.releaseAfterPlay)
			{
				audioEmitterComponent.eventInstanceHandle = audioEngine.InvalidEventInstanceHandle;
			}
		}
		else
		{
			if (audioEmitterComponent.releaseAfterPlay)
			{
				if (attributesValid)
				{
					audioEngine.PlayEvent(audioEmitterComponent.eventName, attributes, audioEmitterComponent.releaseAfterPlay);
				}
				else
				{
					audioEngine.PlayEvent(audioEmitterComponent.eventName, audioEmitterComponent.releaseAfterPlay);
				}
			}
			else
			{
				audioEmitterComponent.eventInstanceHandle = audioEngine.PlayEvent(audioEmitterComponent.eventName, audioEmitterComponent.releaseAfterPlay);
			}
		}
	}
}