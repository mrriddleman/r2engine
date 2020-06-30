#ifndef __RENDERER_IMPL_H__
#define __RENDERER_IMPL_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Model/Material.h"

namespace r2
{
	struct Camera;
}

namespace r2::draw
{
	struct BufferLayoutConfiguration;


}

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
	void Shutdown();
	void SwapScreens();
	void* GetRenderContext();
	void* GetWindowHandle();
	//@TODO(Serge): add more limits of the GPU
	s32 MaxNumberOfTextureUnits();

	//Setup code
	void SetClearColor(const glm::vec4& color);
	void GenerateBufferLayouts(u32 numBufferLayouts, u32* layoutIds);
	void GenerateVertexBuffers(u32 numVertexBuffers, u32* bufferIds);
	void GenerateIndexBuffers(u32 numIndexBuffers, u32* indexIds);
	void SetupBufferLayoutConfiguration(const BufferLayoutConfiguration& config, BufferLayoutHandle layoutId, VertexBufferHandle bufferId, IndexBufferHandle indexId);
	void SetDepthTest(bool shouldDepthTest);

	//
	void UpdateCamera(const r2::Camera& camera);
	void SetViewport(u32 viewport);
	void SetViewportLayer(u32 viewportLayer);
	void SetMaterialID(r2::draw::MaterialHandle materialHandle);


	//draw functions
	void Clear(u32 flags);
	void DrawIndexed(BufferLayoutHandle layoutId, VertexBufferHandle vBufferHandle, IndexBufferHandle iBufferHandle, u32 numIndices, u32 startingIndex);
	void UpdateVertexBuffer(VertexBufferHandle vBufferHandle, u64 offset, void* data, u64 size);
	void UpdateIndexBuffer(IndexBufferHandle iBufferHandle, u64 offset, void* data, u64 size);

	//events
	void WindowResized(u32 width, u32 height);
	void MakeCurrent();
	int SetFullscreen(int flags);
	int SetVSYNC(bool vsync);
	void SetWindowSize(u32 width, u32 height);
}

#endif