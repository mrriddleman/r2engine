#ifndef __AUDIO_EMITTER_COMPONENT_H__
#define __AUDIO_EMITTER_COMPONENT_H__

#include "r2/Utils/Utils.h"
#include "r2/Audio/AudioEngine.h"

namespace r2::ecs
{
	enum AudioEmitterStartCondition : u32
	{
		PLAY_ON_CREATE = 0,
		PLAY_ON_EVENT,
		NUM_AUDIO_EMITTER_START_TYPES
	};

	struct AudioEmitterParameter
	{
		char parameterName[r2::fs::FILE_PATH_LENGTH];
		float parameterValue;
	};

	const u32 MAX_AUDIO_EMITTER_PARAMETERS = 8;

	struct AudioEmitterComponent
	{
		r2::audio::AudioEngine::EventInstanceHandle eventInstanceHandle;
		char eventName[r2::fs::FILE_PATH_LENGTH];
		AudioEmitterParameter parameters[MAX_AUDIO_EMITTER_PARAMETERS];
		u32 numParameters;
		AudioEmitterStartCondition startCondition;
		b32 allowFadeoutWhenStopping;
		b32 releaseAfterPlay;
	};
}

#endif