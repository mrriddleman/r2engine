#ifndef __CHARACTER_CONTROLLER_SYSTEM_H__
#define __CHARACTER_CONTROLLER_SYSTEM_H__


#include "r2/Game/ECS/System.h"
#include <glm/glm.hpp>

struct PlayerCommandComponent;

class CharacterControllerSystem : public r2::ecs::System
{
public:
	CharacterControllerSystem();
	virtual void Update() override;

private:

	
	

};

#endif // __CHARACTER_CONTROLLER_SYSTEM_H__
