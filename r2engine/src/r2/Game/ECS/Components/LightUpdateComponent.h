#ifndef __LIGHT_UPDATE_COMPONENT_H__
#define __LIGHT_UPDATE_COMPONENT_H__

#include "r2/Utils/Flags.h"

namespace r2::ecs
{

	enum eLightUpdateFlags : u32
	{
		DIRECTION_LIGHT_UPDATE = 1 << 0,
		POINT_LIGHT_UPDATE = 1 << 1,
		SPOT_LIGHT_UPDATE = 1 << 2
	};

	struct LightUpdateComponent
	{
		r2::Flags<eLightUpdateFlags, u32> flags;
	};
}


#endif