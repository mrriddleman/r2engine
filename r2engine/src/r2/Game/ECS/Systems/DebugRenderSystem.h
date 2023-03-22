#ifndef __DEBUG_RENDER_SYSTEM_H__
#define __DEBUG_RENDER_SYSTEM_H__

#ifdef R2_DEBUG

#include "r2/Game/ECS/System.h"

namespace r2::ecs
{
	struct DebugRenderComponent;
	struct TransformComponent;
	struct InstanceComponent;

	class DebugRenderSystem : public System
	{
	public:
		DebugRenderSystem();
		~DebugRenderSystem();
		void Render();

	private:
		void RenderDebug(const DebugRenderComponent& c, const TransformComponent& t);
		void RenderDebugInstanced(const DebugRenderComponent& c, const TransformComponent& t, const InstanceComponent& instanceComponent);
	};
}


#endif
#endif // __DEBUG_RENDER_SYSTEM_H__
