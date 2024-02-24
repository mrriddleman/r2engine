#ifndef __DEBUG_BONES_RENDER_SYSTEM_H__
#define __DEBUG_BONES_RENDER_SYSTEM_H__

#ifdef R2_DEBUG

#include "r2/Game/ECS/System.h"

namespace r2::ecs
{
	class DebugBonesRenderSystem : public System
	{
	public:
		DebugBonesRenderSystem();
		~DebugBonesRenderSystem();

		void Render() override;
	};
}

#endif
#endif
