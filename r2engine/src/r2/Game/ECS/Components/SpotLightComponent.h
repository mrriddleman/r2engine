#ifndef __SPOT_LIGHT_COMPONENT_H__
#define __SPOT_LIGHT_COMPONENT_H__

#include "r2/Render/Model/Light.h"

namespace r2::ecs
{
	struct SpotLightComponent
	{
		r2::draw::SpotLightHandle spotLightHandle; //handle to the created point light in the light system
		r2::draw::LightProperties lightProperties; //This is what's serialized
		float innerCutoffAngle;
		float outerCutoffAngle;
		//Maybe will need direction - hopefully we can get away with the transform component instead
	};
}

#endif