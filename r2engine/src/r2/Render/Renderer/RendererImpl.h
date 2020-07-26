#ifndef __RENDERER_IMPL_H__
#define __RENDERER_IMPL_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Model/Material.h"
#include "r2/Render/Renderer/BufferLayout.h"

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
	void SetWindowName(const char* windowName);
	//should be called by the renderer
	bool RendererImplInit(const r2::mem::MemoryArea::Handle memoryAreaHandle, u64 numRingBuffers, u64 maxRingLocks, const char* systemName);

	u64 MemorySize(u64 numRingBuffers, u64 maxNumRingLocks);

	void Shutdown();
	void SwapScreens();
	void* GetRenderContext();
	void* GetWindowHandle();
	//@TODO(Serge): add more limits of the GPU
	s32 MaxNumberOfTextureUnits();
	u32 MaxConstantBufferSize();
	u32 MaxConstantBufferPerShaderType();
	u32 MaxConstantBindings();


	//Setup code
	void SetClearColor(const glm::vec4& color);
	void GenerateBufferLayouts(u32 numBufferLayouts, u32* layoutIds);
	void GenerateVertexBuffers(u32 numVertexBuffers, u32* bufferIds);
	void GenerateIndexBuffers(u32 numIndexBuffers, u32* indexIds);
	void GenerateContantBuffers(u32 numConstantBuffers, u32* contantBufferIds);
	void SetupBufferLayoutConfiguration(const BufferLayoutConfiguration& config, BufferLayoutHandle layoutId, VertexBufferHandle bufferId, IndexBufferHandle indexId, DrawIDHandle drawId);
	void SetupConstantBufferConfigs(const r2::SArray<r2::draw::ConstantBufferLayoutConfiguration>*configs, ConstantBufferHandle* handles);
	void SetDepthTest(bool shouldDepthTest);
	void DeleteBuffers(u32 numBuffers, u32* bufferIds);

	//
	void SetViewport(u32 viewport);
	void SetViewportLayer(u32 viewportLayer);
	void SetMaterialID(r2::draw::MaterialHandle materialHandle);


	//draw functions
	void Clear(u32 flags);
	void DrawIndexed(BufferLayoutHandle layoutId, VertexBufferHandle vBufferHandle, IndexBufferHandle iBufferHandle, u32 numIndices, u32 startingIndex);
	void DrawIndexedCommands(BufferLayoutHandle layoutId, ConstantBufferHandle batchHandle, void* cmds, u32 count, u32 stride = 0);
	void UpdateVertexBuffer(VertexBufferHandle vBufferHandle, u64 offset, void* data, u64 size);
	void UpdateIndexBuffer(IndexBufferHandle iBufferHandle, u64 offset, void* data, u64 size);
	void UpdateConstantBuffer(ConstantBufferHandle cBufferHandle, r2::draw::ConstantBufferLayout::Type type, b32 isPersistent, u64 offset, void* data, u64 size);

	//events
	void WindowResized(u32 width, u32 height);
	void MakeCurrent();
	int SetFullscreen(int flags);
	int SetVSYNC(bool vsync);
	void SetWindowSize(u32 width, u32 height);
}

#endif