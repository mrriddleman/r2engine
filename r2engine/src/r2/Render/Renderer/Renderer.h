
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "r2/Core/Memory/Memory.h"

namespace r2::draw::renderer
{
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