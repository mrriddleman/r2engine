#ifndef __AUDIO_LISTENER_H__
#define __AUDIO_LISTENER_H__

#include "r2/Audio/AudioEngine.h"
#include "r2/Game/ECS/Entity.h"

namespace r2::ecs 
{
	struct AudioListenerComponent
	{
		r2::audio::AudioEngine::Listener listener;
		r2::ecs::Entity entityToFollow;
	};
}

#endif