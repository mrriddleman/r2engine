#ifndef __DEBUG_RENDER_SYSTEM_H__
#define __DEBUG_RENDER_SYSTEM_H__

#ifdef R2_DEBUG

#include "r2/Game/ECS/System.h"

namespace r2::ecs
{
	struct DebugRenderComponent;
	struct TransformComponent;
	template<typename T> struct InstanceComponentT;


	class DebugRenderSystem : public System
	{
	public:
		DebugRenderSystem();
		~DebugRenderSystem();
		void Render() override;

	private:
		void RenderDebug(const DebugRenderComponent& c, const TransformComponent& t);
		void RenderDebugInstanced(
			const DebugRenderComponent& c,
			const TransformComponent& t,
			const InstanceComponentT<DebugRenderComponent>& instancedDebugRenderComponent,
			const InstanceComponentT<TransformComponent>& instancedTransformComponent);
	};
}


#endif
#endif // __DEBUG_RENDER_SYSTEM_H__
