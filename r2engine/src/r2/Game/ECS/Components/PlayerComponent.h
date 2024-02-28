#ifndef __PLAYER_COMPONENT_H__
#define __PLAYER_COMPONENT_H__

#include "r2/Platform/IO.h"
#include "r2/Game/Player/Player.h"

namespace r2::ecs
{
	
	struct PlayerComponent
	{
		PlayerID playerID;
	//	r2::io::InputType inputType;
	};
}

#endif
