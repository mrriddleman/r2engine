#ifndef __RENDERER_IMPL_H__
#define __RENDERER_IMPL_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Model/Material.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Renderer/Commands.h"


#define MAX_RENDER_TARGETS r2::draw::cmd::SetRenderTargetMipLevel::MAX_NUMBER_OF_TEXTURE_ATTACHMENTS

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
	void SetDepthClearColor(float color);
	void GenerateBufferLayouts(u32 numBufferLayouts, u32* layoutIds);
	void GenerateVertexBuffers(u32 numVertexBuffers, u32* bufferIds);
	void GenerateIndexBuffers(u32 numIndexBuffers, u32* indexIds);
	void DeleteBufferLayouts(u32 numBufferLayouts, u32* layoutIds);
	void DeleteVertexBuffers(u32 numVertexBuffers, u32* vertexBufferIds);
	void DeleteIndexBuffers(u32 numIndexBuffers, u32* indexIds);
	
	void AllocateVertexBufferWithData(u32 bufferHandle, u32 size, u32 drawType, void* data);
	void AllocateVertexBuffer(u32 bufferHandle, u32 size, u32 drawType);
	void AllocateIndexBuffer(u32 bufferHandle, u32 size, u32 drawType);

	void GenerateContantBuffers(u32 numConstantBuffers, u32* contantBufferIds);
	void SetupBufferLayoutConfiguration(const BufferLayoutConfiguration& config, BufferLayoutHandle layoutId, VertexBufferHandle vertexBufferId[], u32 numVertexBufferHandles, IndexBufferHandle indexBufferId, DrawIDHandle drawId);
	void SetupConstantBufferConfigs(const r2::SArray<r2::draw::ConstantBufferLayoutConfiguration>* configs, ConstantBufferHandle* handles);
	void SetDepthTest(bool shouldDepthTest);
	void SetDepthWriteEnabled(bool depthWriteEnabled);
	void DeleteBuffers(u32 numBuffers, const u32* bufferIds);
	void SetCullState(const CullState& cullState);
	void SetDepthFunction(u32 depthFunc);
	void EnablePolygonOffset(bool enabled);
	void SetPolygonOffset(const glm::vec2& polygonOffset);
	void SetStencilState(const StencilState& stencilState);
	void SetBlendState(const BlendState& blendState);

	s32 GetConstantLocation(ShaderHandle shaderHandle, const char* name);

	void SetDepthClamp(bool shouldDepthClamp);

	//
	void SetViewportKey(u32 viewport);
	void SetViewportLayer(u32 viewportLayer);
	void SetMaterialID(r2::draw::MaterialHandle materialHandle);
	void SetShaderID(r2::draw::ShaderHandle shaderHandle);

	//Command functions
	void Clear(u32 flags);
	void DrawIndexed(BufferLayoutHandle layoutId, VertexBufferHandle vBufferHandle, IndexBufferHandle iBufferHandle, u32 numIndices, u32 startingIndex);
	void DrawIndexedCommands(BufferLayoutHandle layoutId, ConstantBufferHandle batchHandle, void* cmds, u32 count, u32 offset, u32 stride = 0, PrimitiveType primitivetype = PrimitiveType::TRIANGLES);
	void UpdateVertexBuffer(VertexBufferHandle vBufferHandle, u64 offset, void* data, u64 size);
	void UpdateIndexBuffer(IndexBufferHandle iBufferHandle, u64 offset, void* data, u64 size);
	void CopyBuffer(u32 readBuffer, u32 writeBuffer, u32 readOffset, u32 writeOffset, u32 size);
	void UpdateConstantBuffer(ConstantBufferHandle cBufferHandle, r2::draw::ConstantBufferLayout::Type type, b32 isPersistent, u64 offset, void* data, u64 size);
	void CompleteConstantBuffer(ConstantBufferHandle cBufferHandle, u64 totalSize);
	void DrawDebugCommands(BufferLayoutHandle layoutId, ConstantBufferHandle batchHandle, void* cmds, u32 count, u32 offset, u32 stride = 0);
	void ApplyDrawState(const cmd::DrawState& state);
	void DispatchComputeIndirect(ConstantBufferHandle dispatchBufferHandle, u32 offset);
	void DispatchCompute(u32 numGroupX, u32 numGroupY, u32 numGroupZ);
	void Barrier(u32 flags);
	void ConstantUint(u32 uniformLocation, u32 value);
	
	void SetRenderTargetMipLevel(
		u32 fboHandle,
		const s32 colorTextures[MAX_RENDER_TARGETS],
		const u32 colorMipLevels[MAX_RENDER_TARGETS],
		const u32 colorTextureLayers[MAX_RENDER_TARGETS],
		u32 numColorTextures,
		s32 depthTexture,
		u32 depthMipLevel,
		u32 depthTextureLayer,
		s32 stencilTexture,
		u32 stencilMipLevel,
		u32 stencilTextureLayer,
		s32 depthStencilTexture,
		u32 depthStencilMipLevel,
		u32 depthStencilTextureLayer,
		u32 xOffset,
		u32 yOffset,
		u32 width,
		u32 height,
		b32 colorUseLayeredRenderering,
		b32 depthUseLayeredRenderering,
		b32 stencilUseLayeredRenderering,
		b32 depthStencilUseLayeredRenderering,
		b32 colorIsMSAA, 
		b32 depthStencilIsMSAA);

	void CopyRenderTargetColorTexture(u32 fboHandle, u32 attachmentIndex, u32 textureID, u32 mipLevel, s32 xOffset, s32 yOffset, s32 layer, s32 x, s32 y, u32 width, u32 height);
	
	void SetTexture(u32 textureContainerUniformLocation, u64 textureContainer, u32 texturePageUniformLocation, float texturePage, u32 textureLodUniformLocation, float textureLod);

	void BindImageTexture(u32 textureUnit, u32 texture, u32 mipLevel, b32 layered, u32 layer, u32 access, u32 format);

	void BlitFramebuffer(u32 readFramebuffer, u32 drawFramebuffer, s32 srcX0, s32 srcY0, s32 srcX1, s32 srcY1, s32 dstX0, s32 dstY0, s32 dstX1, s32 dstY1, u32 mask, u32 filter);

	//events
	void SetWindowSize(u32 width, u32 height);
	void SetWindowPosition(s32 xPos, s32 yPos);
	void CenterWindow();

	void MakeCurrent();
	int SetFullscreen(int flags);
	int SetVSYNC(bool vsync);
	void SetViewport(u32 xOffset, u32 yOffset, u32 width, u32 height);
}

#endif