#ifndef __CHARACTER_MOVEMENT_SYSTEM_H__
#define __CHARACTER_MOVEMENT_SYSTEM_H__

#include "r2/Game/ECS/System.h"

class CharacterMovementSystem : public r2::ecs::System
{
public:
	CharacterMovementSystem();
	virtual void Update() override;
	virtual void Render() override;

};


#endif