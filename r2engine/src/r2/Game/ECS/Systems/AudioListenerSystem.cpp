#include "r2pch.h"
#include "r2/Game/ECS/Systems/AudioListenerSystem.h"

#include "r2/Audio/AudioEngine.h"
#include "r2/Game/ECS/Components/AudioListenerComponent.h"
#include "r2/Game/ECS/Components/AudioEmitterComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"

namespace r2::ecs
{

	AudioListenerSystem::AudioListenerSystem()
	{
		mKeepSorted = false;
	}

	AudioListenerSystem::~AudioListenerSystem()
	{

	}

	void AudioListenerSystem::Update()
	{
		R2_CHECK(mnoptrCoordinator != nullptr, "Not sure why this should ever be nullptr?");
		R2_CHECK(mEntities != nullptr, "Not sure why this should ever be nullptr?");

		r2::audio::AudioEngine audioEngine;

		const auto numEntities = r2::sarr::Size(*mEntities);

		//first set the number of active listeners if that has changed
		const auto numListeners = audioEngine.GetNumListeners();

		if (numListeners != numEntities && numEntities > 0)
		{
			audioEngine.SetNumListeners(numEntities);
		}

		//now update the 3D attributes
		for (u32 i = 0; i < numEntities; ++i)
		{
			Entity e = r2::sarr::At(*mEntities, i);

			AudioListenerComponent& listenerComponent = mnoptrCoordinator->GetComponent<AudioListenerComponent>(e);
			TransformComponent& transformComponent = mnoptrCoordinator->GetComponent<TransformComponent>(e);

			TransformDirtyComponent* transformDirtyComponent = mnoptrCoordinator->GetComponentPtr<TransformDirtyComponent>(e);

			bool needsUpdate = transformDirtyComponent != nullptr;

			if (listenerComponent.entityToFollow != r2::ecs::INVALID_ENTITY)
			{
				TransformDirtyComponent* followTransformDirtyComponent = mnoptrCoordinator->GetComponentPtr<TransformDirtyComponent>(listenerComponent.entityToFollow);
				needsUpdate = followTransformDirtyComponent != nullptr;
			}

			if (!needsUpdate)
			{
				continue;
			}

			glm::vec3 upVec = transformComponent.accumTransform.rotation * glm::vec3(0, 0, 1);
			glm::vec3 forwardVec = transformComponent.accumTransform.rotation * glm::vec3(0, 1, 0);
			glm::vec3 position = transformComponent.accumTransform.position;

			if (listenerComponent.entityToFollow != r2::ecs::INVALID_ENTITY)
			{
				TransformComponent& followTransformComponent = mnoptrCoordinator->GetComponent<TransformComponent>(listenerComponent.entityToFollow);

				upVec = followTransformComponent.accumTransform.rotation * glm::vec3(0, 0, 1);
				forwardVec = followTransformComponent.accumTransform.rotation * glm::vec3(0, 1, 0);
				position = followTransformComponent.accumTransform.position;
			}

			r2::audio::AudioEngine::Attributes3D attributes;
			attributes.up = upVec;
			attributes.look = forwardVec;
			attributes.velocity = glm::vec3(0); //@TODO(Serge): maybe have a velocity component later...
			attributes.position = position;

			audioEngine.SetListener3DAttributes(listenerComponent.listener, attributes);
		}
	}

}