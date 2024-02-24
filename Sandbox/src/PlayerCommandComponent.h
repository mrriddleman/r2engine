#ifndef __PLAYER_COMMAND_COMPONENT_H__
#define __PLAYER_COMMAND_COMPONENT_H__

#include "r2/Game/Player/Player.h"

struct ActionButton
{
	
	b32 enabledThisFrame;
	b32 isEnabled;
};

struct PlayerCommandComponent
{
	r2::PlayerID playerID;

	ActionButton leftAction;
	ActionButton rightAction;
	ActionButton upAction;
	ActionButton downAction;

	ActionButton grabAction;
	//ActionButton undoAction;
};


#endif // __PLAYER_COMMAND_COMPONENT_H__
