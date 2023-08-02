#ifndef __AUDIO_EMITTER_SYSTEM_H__
#define __AUDIO_EMITTER_SYSTEM_H__

#include "r2/Game/ECS/System.h"
#include "r2/Audio/AudioEngine.h"

namespace r2::ecs
{
	struct AudioEmitterComponent;

	class AudioEmitterSystem : public System
	{
	public:
		AudioEmitterSystem();
		~AudioEmitterSystem();

		void Update();

	private:
		void PlayEvent(AudioEmitterComponent& audioEmitterComponent, const r2::audio::AudioEngine::Attributes3D& attributes, bool attributesValid);
	};
}

#endif
