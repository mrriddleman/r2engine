#ifndef __RENDER_TARGET_H__
#define __RENDER_TARGET_H__

#include "r2/Utils/Utils.h"
#include "r2/Render/Model/Textures/Texture.h"

namespace r2::draw::rt
{
	constexpr u32 MAX_TEXTURE_ATTACHMENT_HISTORY = 2;

	struct TextureAttachment
	{
		tex::TextureHandle texture[MAX_TEXTURE_ATTACHMENT_HISTORY];
		u32 numLayers = 1;
		u32 numTextures = 1;
		u32 currentTexture = 0;

		b32 uploadAllTextures = false;
		b32 needsFramebufferUpdate = false;
		u32 mipLevelAttached = 0;
		b32 useLayeredRenderering = false;

		u32 colorAttachmentNumber = 0;

		tex::TextureFormat format;
	};

	struct RenderBufferAttachment
	{
		u32 rbo = 0;
		s32 attachmentType = -1;
	};

	enum TextureAttachmentType: u32
	{
		COLOR = 0,
		DEPTH,
		DEPTH_CUBEMAP,
		RG16F,
		RG32F,
		R32F,
		R16F,
		RG16,
		STENCIL8,
		DEPTH24_STENCIL8,
		DEPTH32F_STENCIL8
	};

	struct RenderTargetPageAllocation
	{
		rt::TextureAttachmentType type;
		u32 attachmentIndex;
		float sliceIndex;
		u32 numPages;
	};

	struct TextureAttachmentReference
	{
		TextureAttachmentType type;
		const TextureAttachment* attachmentPtr;
		u32 colorAttachmentNumber = 0;
	};

	struct RenderTargetParams
	{
		u32 numColorAttachments;
		u32 numDepthAttachments;
		u32 numStencilAttachments;
		u32 numDepthStencilAttachments;
		u32 numRenderBufferAttachments;
		u32 maxPageAllocations;
		u32 numAttachmentRefs;

		u32 surfaceOffset;
		u32 numSurfacesPerTarget;
	};
}

namespace r2::draw
{
	enum RenderTargetSurface : s8
	{
		RTS_EMPTY = -1,
		
		RTS_GBUFFER,
		RTS_SHADOWS,
		RTS_COMPOSITE,
		RTS_ZPREPASS,
		RTS_POINTLIGHT_SHADOWS,
		RTS_AMBIENT_OCCLUSION,
		RTS_AMBIENT_OCCLUSION_DENOISED,
		RTS_ZPREPASS_SHADOWS,
		RTS_AMBIENT_OCCLUSION_TEMPORAL_DENOISED,
		RTS_NORMAL,
		RTS_SPECULAR,
		RTS_SSR,
		RTS_CONVOLVED_GBUFFER,
		RTS_SSR_CONE_TRACED,
		RTS_BLOOM,
		RTS_BLOOM_BLUR,
		RTS_BLOOM_UPSAMPLE,
		NUM_RENDER_TARGET_SURFACES,
	};

	struct RenderTarget
	{
		static const u32 DEFAULT_RENDER_TARGET;
		u32 frameBufferID = DEFAULT_RENDER_TARGET;
		u32 width = 0, height = 0;
		u32 xOffset = 0, yOffset = 0;
		u32 numFrameBufferColorAttachments = 0;

		r2::SArray<rt::TextureAttachment>* colorAttachments = nullptr;

		r2::SArray<rt::TextureAttachment>* depthAttachments = nullptr;

		r2::SArray<rt::TextureAttachment>* stencilAttachments = nullptr;

		r2::SArray<rt::TextureAttachment>* depthStencilAttachments = nullptr;
		
		r2::SArray<rt::RenderBufferAttachment>* renderBufferAttachments = nullptr;

		r2::SArray<rt::RenderTargetPageAllocation>* pageAllocations = nullptr;

		r2::SArray<rt::TextureAttachmentReference>* attachmentReferences = nullptr;

		static u64 RenderTarget::MemorySize(const rt::RenderTargetParams& params, u64 alignmnet, u32 headerSize, u32 boundsChecking);
	};

	namespace cmd
	{
		struct SetRenderTargetMipLevel;
		void FillSetRenderTargetMipLevelCommand(const RenderTarget& rt, u32 mipLevel, SetRenderTargetMipLevel& cmd);
		void FillSetRenderTargetMipLevelCommandWithTextureIndex(const RenderTarget& rt, u32 mipLevel, u32 textureIndex, SetRenderTargetMipLevel& cmd);
	}

	namespace rt
	{

		extern s32 COLOR_ATTACHMENT;
		extern s32 DEPTH_ATTACHMENT;
		extern s32 DEPTH_STENCIL_ATTACHMENT;
		extern s32 STENCIL_ATTACHMENT;
		extern s32 MSAA_ATTACHMENT;

		template <class ARENA>
		RenderTarget CreateRenderTarget(ARENA& arena, const RenderTargetParams& params, u32 xOffset, u32 yOffset, u32 width, u32 height, const char* file, s32 line, const char* description);

		template <class ARENA>
		void DestroyRenderTarget(ARENA& arena, RenderTarget& rt);

		void AddTextureAttachment(RenderTarget& rt, TextureAttachmentType type, s32 filter, s32 wrapMode, u32 layers, s32 mipLevels, bool alpha, bool isHDR, bool useLayeredRendering);

		void AddTextureAttachment(RenderTarget& rt,
			TextureAttachmentType type,
			bool swapping, bool uploadAllTextures,
			s32 filter, s32 wrapMode, u32 layers, s32 mipLevels, bool alpha, bool isHDR, bool useLayeredRendering, u32 mipLevelToAttach);

		void SetTextureAttachment(RenderTarget& rt, TextureAttachmentType type, const rt::TextureAttachment& textureAttachment);

		//returns the first index of the number of texture pages
		float AddTexturePagesToAttachment(RenderTarget& rt, TextureAttachmentType type, u32 pages);

		void RemoveTexturePagesFromAttachment(RenderTarget& rt, TextureAttachmentType type, float index, u32 pages);

		void AddDepthAndStencilAttachment(RenderTarget& rt);

		void SwapTexturesIfNecessary(RenderTarget& rt);

	//	void UpdateRenderTargetIfNecessary(RenderTarget& rt);

		bool IsColorAttachment(TextureAttachmentType type);

		bool IsDepthAttachment(TextureAttachmentType type);

		bool IsStencilAttachment(TextureAttachmentType type);

		bool IsDepthStencilAttachment(TextureAttachmentType type);

		//private
		namespace impl
		{
			void AddTextureAttachment(RenderTarget& rt, TextureAttachmentType type, bool swapping, bool uploadAllTextures, s32 filter, s32 wrapMode, u32 layers, s32 mipLevels, bool alpha, bool isHDR, bool useLayeredRendering, u32 mipLevelToAttach);
			void SetTextureAttachment(RenderTarget& rt, TextureAttachmentType type, const rt::TextureAttachment& textureAttachment);
			float AddTexturePagesToAttachment(RenderTarget& rt, TextureAttachmentType type, u32 pages);
			void RemoveTexturePagesFromAttachment(RenderTarget& rt, TextureAttachmentType type, float index, u32 pages);
			void AddDepthAndStencilAttachment(RenderTarget& rt);
			void CreateFrameBufferID(RenderTarget& renderTarget);
			void DestroyFrameBufferID(RenderTarget& renderTarget);
			void SwapTexturesIfNecessary(RenderTarget& renderTarget);
		//	void UpdateRenderTargetIfNecessary(RenderTarget& renderTarget);
		}

	}

	namespace rt
	{
		template <class ARENA>
		RenderTarget CreateRenderTarget(ARENA& arena, const RenderTargetParams& params, u32 xOffset, u32 yOffset, u32 width, u32 height, const char* file, s32 line, const char* description)
		{
			RenderTarget rt;

			if (params.numColorAttachments > 0)
			{
				rt.colorAttachments = MAKE_SARRAY_VERBOSE(arena, TextureAttachment, params.numColorAttachments, file, line, description);
			}

			if (params.numDepthAttachments > 0)
			{
				rt.depthAttachments = MAKE_SARRAY_VERBOSE(arena, TextureAttachment, params.numDepthAttachments, file, line, description);
			}
			
			if (params.numRenderBufferAttachments > 0)
			{
				rt.renderBufferAttachments = MAKE_SARRAY_VERBOSE(arena, RenderBufferAttachment, params.numRenderBufferAttachments, file, line, description);
			}
			
			if (params.maxPageAllocations > 0)
			{
				rt.pageAllocations = MAKE_SARRAY_VERBOSE(arena, RenderTargetPageAllocation, params.maxPageAllocations, file, line, description);
			}

			if (params.numAttachmentRefs > 0)
			{
				rt.attachmentReferences = MAKE_SARRAY_VERBOSE(arena, TextureAttachmentReference, params.numAttachmentRefs, file, line, description);
			}

			if (params.numStencilAttachments > 0)
			{
				rt.stencilAttachments = MAKE_SARRAY_VERBOSE(arena, TextureAttachment, params.numStencilAttachments, file, line, description);
			}

			if (params.numDepthStencilAttachments > 0)
			{
				rt.depthStencilAttachments = MAKE_SARRAY_VERBOSE(arena, TextureAttachment, params.numDepthStencilAttachments, file, line, description);
			}

			rt.width = width;
			rt.height = height;
			rt.xOffset = xOffset;
			rt.yOffset = yOffset;
			//Make the actual frame buffer
			impl::CreateFrameBufferID(rt);

			return rt;
		}


		template <class ARENA>
		void DestroyRenderTarget(ARENA& arena, RenderTarget& rt)
		{

			//Free all of the page allocations
			if (rt.pageAllocations)
			{
				const auto numPageAllocations = r2::sarr::Size(*rt.pageAllocations);

				for (u64 i = 0; i < numPageAllocations; ++i)
				{
					const RenderTargetPageAllocation& page = r2::sarr::At(*rt.pageAllocations, i);
					RemoveTexturePagesFromAttachment(rt, page.type, page.sliceIndex, page.numPages);
				}
			}

			impl::DestroyFrameBufferID(rt);

			if (rt.depthStencilAttachments)
			{
				FREE(rt.depthStencilAttachments, arena);
			}

			if (rt.stencilAttachments)
			{
				FREE(rt.stencilAttachments, arena);
			}

			if (rt.attachmentReferences)
			{
				FREE(rt.attachmentReferences, arena);
			}

			if (rt.pageAllocations)
			{
				r2::sarr::Clear(*rt.pageAllocations);
				FREE(rt.pageAllocations, arena);
			}

			if (rt.renderBufferAttachments)
			{
				FREE(rt.renderBufferAttachments, arena);
			}
			
			if (rt.depthAttachments)
			{
				FREE(rt.depthAttachments, arena);
			}

			if (rt.colorAttachments)
			{
				FREE(rt.colorAttachments, arena);
			}
		}

	}
}


#endif // ! __RENDER_TARGET_H__
