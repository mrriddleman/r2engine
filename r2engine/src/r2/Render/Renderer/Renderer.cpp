#include "r2pch.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Render/Renderer/RendererImpl.h"
namespace r2::draw::renderer
{
	//basic stuff

	bool Init(const r2::mem::utils::MemBoundary& memoryBoundary)
	{
		return r2::draw::rendererimpl::Init(memoryBoundary);
	}

	void Render(float alpha)
	{
		r2::draw::rendererimpl::Render(alpha);
	}

	void Shutdown()
	{
		r2::draw::rendererimpl::Shutdown();
	}

	//events
	void WindowResized(u32 width, u32 height)
	{
		r2::draw::rendererimpl::WindowResized(width, height);
	}

	void MakeCurrent()
	{
		r2::draw::rendererimpl::MakeCurrent();
	}

	int SetFullscreen(int flags)
	{
		return r2::draw::rendererimpl::SetFullscreen(flags);
	}

	int SetVSYNC(bool vsync)
	{
		return r2::draw::rendererimpl::SetVSYNC(vsync);
	}

	void SetWindowSize(u32 width, u32 height)
	{
		r2::draw::rendererimpl::SetWindowSize(width, height);
	}
}