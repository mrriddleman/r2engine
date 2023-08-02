#ifndef __AUDIO_EMITTER_ACTION_COMPONENT_H__
#define __AUDIO_EMITTER_ACTION_COMPONENT_H__

#include "r2/Audio/AudioEngine.h"
#include "r2/Utils/Utils.h"

namespace r2::ecs 
{

	enum AudioEmitterAction : u32
	{
		AEA_CREATE = 0,
		AEA_PLAY,
		AEA_STOP,
		AEA_PAUSE,
		AEA_DESTROY,
		NUM_AUDIO_EMITTER_ACTIONS
	};

	struct AudioEmitterActionComponent
	{
		r2::audio::AudioEngine::EventInstanceHandle eventInstanceHandle;
		AudioEmitterAction action;
	};
}

#endif
