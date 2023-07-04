#ifndef __ECS_LAYER_H__
#define __ECS_LAYER_H__

#include "r2/Core/Layer/Layer.h"

namespace r2::ecs
{
	class ECSWorld;
}

namespace r2
{
	class ECSLayer : public Layer
	{
	public:
		ECSLayer(r2::ecs::ECSWorld* noptrECSWorld);
		virtual void Init() override;
		virtual void Shutdown()  override;
		virtual void Update()  override;
		virtual void ImGuiRender(u32 dockingSpaceID)  override;
		virtual void Render(float alpha)  override;
		virtual void OnEvent(evt::Event& event)  override;

	private:
		r2::ecs::ECSWorld* mnoptrECSWorld;
	};
}

#endif // __ECS_LAYER_H__
