#ifndef __POINT_LIGHT_COMPONENT_H__
#define __POINT_LIGHT_COMPONENT_H__

#include "r2/Render/Model/Light.h"

namespace r2::ecs
{
	struct PointLightComponent
	{
		r2::draw::PointLightHandle pointLightHandle; //handle to the created point light in the light system
		r2::draw::LightProperties lightProperties; //This is what's serialized
	};
}


#endif