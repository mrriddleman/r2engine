#include "r2pch.h"
#include "r2/Core/Layer/ECSLayer.h"
#include "r2/Game/ECSWorld/ECSWorld.h"

namespace r2
{

	ECSLayer::ECSLayer(r2::ecs::ECSWorld* noptrECSWorld)
		:Layer("ECSLayer", true)
		,mnoptrECSWorld(noptrECSWorld)
	{
		R2_CHECK(mnoptrECSWorld != nullptr, "We must have a valid ECSWorld");
	}

	void ECSLayer::Init()
	{

	}

	void ECSLayer::Shutdown()
	{
		mnoptrECSWorld = nullptr;
	}

	void ECSLayer::Update()
	{
		mnoptrECSWorld->Update();
	}

	void ECSLayer::ImGuiRender(u32 dockingSpaceID)
	{

	}

	void ECSLayer::Render(float alpha)
	{
		mnoptrECSWorld->Render();
	}

	void ECSLayer::OnEvent(evt::Event& event)
	{

	}

}