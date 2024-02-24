#ifndef __LIGHT_UPDATE_SYSTEM_H__
#define __LIGHT_UPDATE_SYSTEM_H__

#include "r2/Game/ECS/System.h"

namespace r2::ecs
{	
	class LightingUpdateSystem : public System
	{
	public:

		LightingUpdateSystem();
		~LightingUpdateSystem();
		void Update() override;

	private:

		
	};
}


#endif