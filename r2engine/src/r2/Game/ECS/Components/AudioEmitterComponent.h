#ifndef __AUDIO_EMITTER_COMPONENT_H__
#define __AUDIO_EMITTER_COMPONENT_H__

#include "r2/Utils/Utils.h"

namespace r2::ecs
{
	enum AudioEmitterStartCondition : u32
	{
		PLAY_ON_START = 0,
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
		char eventName[r2::fs::FILE_PATH_LENGTH];
		AudioEmitterParameter parameters[MAX_AUDIO_EMITTER_PARAMETERS];
		AudioEmitterStartCondition startCondition;
		u32 numParameters;
		b32 allowFadeoutWhenStopping;
		b32 triggerOnce;
	};
}

#endif