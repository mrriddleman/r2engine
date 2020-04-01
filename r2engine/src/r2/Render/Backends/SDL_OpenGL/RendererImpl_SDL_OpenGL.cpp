#include "r2pch.h"

#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "r2/Render/Renderer/RendererImpl.h"
#include <SDL2/SDL.h>
#include "glad/glad.h"

namespace
{
	SDL_Window* s_optrWindow = nullptr;
	SDL_GLContext s_glContext;
}

namespace r2::draw::rendererimpl
{
	//basic stuff
	bool PlatformInit(const PlatformRendererSetupParams& params)
	{
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

		SDL_GL_LoadLibrary(nullptr);

		s_optrWindow = SDL_CreateWindow("r2engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, params.resolution.width, params.resolution.height, params.platformFlags);

		R2_CHECK(s_optrWindow != nullptr, "We should have a window pointer!");

		s_glContext = SDL_GL_CreateContext(s_optrWindow);

		R2_CHECK(s_glContext != nullptr, "We should have an OpenGL context!");

		gladLoadGLLoader(SDL_GL_GetProcAddress);
		
		if (params.flags.IsSet(VSYNC))
		{
			SDL_GL_SetSwapInterval(1);
		}
		else
		{
			SDL_GL_SetSwapInterval(0);
		}

		return s_optrWindow && s_glContext;
	}

	void* GetRenderContext()
	{
		return s_glContext;
	}

	void* GetWindowHandle()
	{
		return s_optrWindow;
	}

	bool Init(const r2::mem::utils::MemBoundary& memoryBoundary)
	{

		glClearColor(1, 0, 0, 1);

		return true;
	}

	void Render(float alpha)
	{
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void SwapScreens()
	{
		SDL_GL_SwapWindow(s_optrWindow);
	}

	void Shutdown()
	{
		SDL_GL_DeleteContext(s_glContext);

		if (s_optrWindow)
		{
			SDL_DestroyWindow(s_optrWindow);
		}
	}

	//events
	void WindowResized(u32 width, u32 height)
	{
		glViewport(0, 0, width, height);
	}

	void MakeCurrent()
	{
		SDL_GL_MakeCurrent(s_optrWindow, s_glContext);
	}

	int SetFullscreen(int flags)
	{
		return SDL_SetWindowFullscreen(s_optrWindow, flags);
	}

	int SetVSYNC(bool vsync)
	{
		return SDL_GL_SetSwapInterval(static_cast<int>(vsync));
	}

	void SetWindowSize(u32 width, u32 height)
	{
		SDL_SetWindowSize(s_optrWindow, width, height);
	}
}

#endif // R2_PLATFORM_WINDOWS || R2_PLATFORM_MAC
