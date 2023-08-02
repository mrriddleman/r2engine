#ifndef __AUDIO_PARAMETER_TRIGGER_COMPONENT_H__
#define __AUDIO_PARAMETER_TRIGGER_COMPONENT_H__

#include "r2/Utils/Utils.h"
#include "r2/Audio/AudioEngine.h"

namespace r2::ecs
{
	struct AudioParameterComponent
	{
		r2::audio::AudioEngine::EventInstanceHandle eventInstanceHandle;
		char parameterName[r2::fs::FILE_PATH_LENGTH];
		float parameterValue;
	};
}

#endif