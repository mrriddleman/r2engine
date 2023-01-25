#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/BackendDispatch.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Renderer/Shader.h"

namespace r2::draw
{
	struct Mesh;
}

//@TODO(Serge): change these to packets
//Commands will be for groups of packets

namespace r2::draw::cmd
{
	extern u32 CLEAR_COLOR_BUFFER;
	extern u32 CLEAR_DEPTH_BUFFER;
	extern u32 CLEAR_STENCIL_BUFFER;


	extern u32 SHADER_STORAGE_BARRIER_BIT;
	extern u32 FRAMEBUFFER_BARRIER_BIT;
	extern u32 ALL_BARRIER_BITS;
	
	struct DrawState
	{
		DrawLayer layer;
		b32 depthEnabled; //@TODO(Serge): change this to flags (u32)
		u32 depthFunction;
		b32 depthWriteEnabled;

		b32 polygonOffsetEnabled;
		glm::vec2 polygonOffset;

		CullState cullState;
		StencilState stencilState;
		BlendState blendState;
	};

	struct Clear
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		u32 flags;
	};
	static_assert(std::is_pod<Clear>::value == true, "Clear must be a POD.");

	struct DrawIndexed
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		u32 indexCount;
		u32 startIndex;
		u32 baseVertex;

		//need some kind of layout (VAO - OpenGL) 
		r2::draw::BufferLayoutHandle bufferLayoutHandle;
		r2::draw::VertexBufferHandle vertexBufferHandle;
		r2::draw::IndexBufferHandle indexBufferHandle;
	};
	static_assert(std::is_pod<DrawIndexed>::value == true, "DrawIndexed must be a POD.");

	struct FillVertexBuffer
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		r2::draw::VertexBufferHandle vertexBufferHandle;

		u64 offset;
		u64 dataSize;
		void* data;
	};
	static_assert(std::is_pod<FillVertexBuffer>::value == true, "FillVertexBuffer must be a POD.");

	struct FillIndexBuffer
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		r2::draw::IndexBufferHandle indexBufferHandle;

		u64 offset;
		u64 dataSize;
		void* data;
	};
	static_assert(std::is_pod<FillIndexBuffer>::value == true, "FillIndexBuffer must be a POD.");

	struct CopyBuffer
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		//Maybe this should be a specific buffer handle type (Vertex/Index etc)?
		u32 readBuffer;
		u32 writeBuffer;

		u32 readOffset;
		u32 writeOffset;
		u32 size;
	};

	static_assert(std::is_pod<CopyBuffer>::value == true, "CopyBuffer must be a POD.");

	struct DeleteBuffer
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		u32 bufferHandle;
	};

	static_assert(std::is_pod<DeleteBuffer>::value == true, "DeleteBuffer must be a POD.");

	struct FillConstantBuffer
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		r2::draw::ConstantBufferHandle constantBufferHandle;
		r2::draw::ConstantBufferLayout::Type type;
		b32 isPersistent;
		u64 offset;
		u64 dataSize;
		void* data;
	};
	static_assert(std::is_pod<FillConstantBuffer>::value == true, "FillContantBuffer must be a POD.");

	struct CompleteConstantBuffer
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		r2::draw::ConstantBufferHandle constantBufferHandle;
		u64 count;
	};
	static_assert(std::is_pod<CompleteConstantBuffer>::value == true, "CompleteConstantBuffer must be a POD.");


	struct DrawBatchSubCommand
	{
		u32  count;
		u32  instanceCount;
		u32  firstIndex;
		u32  baseVertex;
		u32  baseInstance;
	};

	struct DrawBatch
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		PrimitiveType primitiveType;
		DrawState state;
		r2::draw::BufferLayoutHandle bufferLayoutHandle;
		r2::draw::ConstantBufferHandle batchHandle;

		u32 startCommandIndex;
		u32 numSubCommands;
		DrawBatchSubCommand* subCommands;
	};
	static_assert(std::is_pod<DrawBatch>::value == true, "DrawBatch must be a POD.");

	struct DrawDebugBatchSubCommand
	{
		u32  count;
		u32  instanceCount;
		u32  firstVertex;
		u32  baseInstance;
	};

	struct DrawDebugBatch
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		PrimitiveType primitiveType;
		DrawState state;
		r2::draw::BufferLayoutHandle bufferLayoutHandle;
		r2::draw::ConstantBufferHandle batchHandle;
		u32 startCommandIndex;
		u32 numSubCommands;
		DrawDebugBatchSubCommand* subCommands;
	};
	static_assert(std::is_pod<DrawDebugBatch>::value == true, "DrawDebugBatch must be a POD.");

	//Render target stuff

	struct SetRenderTargetMipLevel
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		static constexpr u32 MAX_NUMBER_OF_TEXTURE_ATTACHMENTS = 4;

		u32 frameBufferID;

		s32 colorTextures[MAX_NUMBER_OF_TEXTURE_ATTACHMENTS];
		u32 toColorMipLevels[MAX_NUMBER_OF_TEXTURE_ATTACHMENTS];
		u32 colorTextureLayers[MAX_NUMBER_OF_TEXTURE_ATTACHMENTS];
		u32 numColorTextures;
		b32 colorUseLayeredRenderering;
		b32 colorIsMSAA;

		s32 depthTexture;
		u32 toDepthMipLevel;
		u32 depthTextureLayer;
		b32 depthUseLayeredRenderering;

		s32 stencilTexture;
		u32 toStencilMipLevel;
		u32 stencilTextureLayer;
		b32 stencilUseLayeredRendering;

		s32 depthStencilTexture;
		u32 toDepthStencilMipLevel;
		u32 depthStencilTextureLayer;
		b32 depthStencilUseLayeredRenderering;
		b32 depthStencilIsMSAA;

		u32 xOffset;
		u32 yOffset;
		u32 width;
		u32 height;
	};

	static_assert(std::is_pod<SetRenderTargetMipLevel>::value == true, "SetRenderTargetMipLevel must be a POD.");

	struct CopyRenderTargetColorTexture
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		u32 frameBufferID;
		u32 attachment;

		u32 toTextureID;
		u32 mipLevel;
		s32 xOffset;
		s32 yOffset;
		s32 layer;

		s32 x, y;
		u32 width, height;
	};

	static_assert(std::is_pod<CopyRenderTargetColorTexture>::value == true, "CopyRenderTargetColorTexture must be a POD.");

	//Compute stuff

	struct DispatchSubCommand
	{
		u32 numGroupsX;
		u32 numGroupsY;
		u32 numGroupsZ;
		u32 pad0; 
	};

	struct DispatchComputeIndirect
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		r2::draw::ConstantBufferHandle dispatchIndirectBuffer;
		u32 offset;
	};

	static_assert(std::is_pod<DispatchComputeIndirect>::value == true, "DispatchComputeIndirect must be a POD.");


	struct DispatchCompute
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		u32 numGroupsX;
		u32 numGroupsY;
		u32 numGroupsZ;
	};

	static_assert(std::is_pod<DispatchCompute>::value == true, "DispatchCompute must be a POD.");


	struct Barrier
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		u32 flags;
	};

	static_assert(std::is_pod<Barrier>::value == true, "Barrier must be a POD.");

	struct ConstantUint
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		u32 uniformLocation;
		u32 value;
	};

	static_assert(std::is_pod<ConstantUint>::value == true, "ConstantUint must be a POD.");

	struct SetTexture
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		
		u32 textureContainerUniformLocation;
		u32 texturePageUniformLocation;
		u32 textureLodUniformLocation;
		u64 textureContainer;
		float texturePage;
		float textureLod;
	};

	static_assert(std::is_pod<SetTexture>::value == true, "BlurTexture must be a POD.");

	//For reading/writing compute images
	struct BindImageTexture
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		u32 textureUnit;
		u32 texture;
		u32 mipLevel;
		b32 layered;
		s32 layer;
		u32 access;
		u32 format;
	};

	static_assert(std::is_pod<BindImageTexture>::value == true, "BindImageTexture must be a POD.");

	struct BlitFramebuffer
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		u32 readFramebuffer;
		u32 drawFramebuffer;
		s32 srcX0;
		s32 srcY0;
		s32 srcX1;
		s32 srcY1;
		s32 dstX0;
		s32 dstY0;
		s32 dstX1;
		s32 dstY1;
		u32 mask;
		u32 filter;
	};

	static_assert(std::is_pod<BlitFramebuffer>::value == true, "BlitFramebuffer must be a POD.");

	u64 LargestCommand();

	void SetDefaultStencilState(StencilState& stencilState);

	void SetDefaultBlendState(BlendState& blendState);

	void SetDefaultCullState(CullState& cullState);
}

#endif