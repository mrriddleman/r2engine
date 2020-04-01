#ifndef __RENDERER_IMPL_H__
#define __RENDERER_IMPL_H__

#include "r2/Core/Memory/Memory.h"

namespace r2::draw::rendererimpl
{
	enum eRendererSetupFlags
	{
		VSYNC = 1 << 0
	};

	using RendererSetupFlags = r2::Flags<eRendererSetupFlags, u32>;

	struct PlatformRendererSetupParams
	{
		util::Size resolution;
		u32 platformFlags = 0;
		RendererSetupFlags flags;

		char* windowName = "";
	};

	//platform
	bool PlatformInit(const PlatformRendererSetupParams& params);
	void SwapScreens();
	void* GetRenderContext();
	void* GetWindowHandle();


	//basic stuff
	bool Init(const r2::mem::utils::MemBoundary& memoryBoundary);
	void Render(float alpha);
	void Shutdown();

	//events
	void WindowResized(u32 width, u32 height);
	void MakeCurrent();
	int SetFullscreen(int flags);
	int SetVSYNC(bool vsync);
	void SetWindowSize(u32 width, u32 height);
}

#endif