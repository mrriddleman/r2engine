#ifndef __AUDIO_SYSTEM_H__
#define __AUDIO_SYSTEM_H__

#include "r2/Game/ECS/System.h"

namespace r2::ecs
{
	class AudioListenerSystem : public System
	{
	public: 
		AudioListenerSystem();
		~AudioListenerSystem();

		void Update();

	private:
	};
}

#endif // __AUDIO_SYSTEM_H__
