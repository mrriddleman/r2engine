#ifndef __FACING_UPDATE_SYSTEM_H__
#define __FACING_UPDATE_SYSTEM_H__

#include "r2/Game/ECS/System.h"

class FacingUpdateSystem : public r2::ecs::System
{
public:
	FacingUpdateSystem();
	virtual void Update() override;
private:
};


#endif // __FACING_UPDATE_SYSTEM_H__
