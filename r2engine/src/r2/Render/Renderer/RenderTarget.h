#ifndef __RENDER_TARGET_H__
#define __RENDER_TARGET_H__

#include "r2/Utils/Utils.h"
#include "r2/Render/Model/Textures/Texture.h"

namespace r2::draw::rt
{
	struct TextureAttachment
	{
		tex::TextureHandle texture;
		u32 numLayers = 1;
	};

	struct RenderBufferAttachment
	{
		u32 rbo = 0;
		s32 attachmentType = -1;
	};

	enum TextureAttachmentType
	{
		COLOR = 0,
		DEPTH
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
		NUM_RENDER_TARGET_SURFACES
	};

	struct RenderTarget
	{
		static const u32 DEFAULT_RENDER_TARGET;
		u32 frameBufferID = DEFAULT_RENDER_TARGET;
		u32 width = 0, height = 0;
		u32 xOffset = 0, yOffset = 0;

		r2::SArray<rt::TextureAttachment>* colorAttachments = nullptr;
		r2::SArray<rt::TextureAttachment>* depthAttachments = nullptr;
		r2::SArray<rt::RenderBufferAttachment>* renderBufferAttachments = nullptr;

		static u64 RenderTarget::MemorySize(u32 numColorAttachments, u32 numDepthAttachments, u32 numRenderBufferAttachments, u64 alignmnet, u32 headerSize, u32 boundsChecking);
	};

	namespace rt
	{

		extern s32 COLOR_ATTACHMENT;
		extern s32 DEPTH_ATTACHMENT;
		extern s32 DEPTH_STENCIL_ATTACHMENT;
		extern s32 MSAA_ATTACHMENT;

		template <class ARENA>
		RenderTarget CreateRenderTarget(ARENA& arena, u32 maxNumColorAttachments, u32 maxNumDepthAttachments, u32 maxNumRenderBufferAttachments, u32 xOffset, u32 yOffset, u32 width, u32 height, const char* file, s32 line, const char* description);

		template <class ARENA>
		void DestroyRenderTarget(ARENA& arena, RenderTarget& rt);

		void AddTextureAttachment(RenderTarget& rt, TextureAttachmentType type, s32 filter, s32 wrapMode, u32 layers, s32 mipLevels, bool alpha, bool isHDR);

		void AddDepthAndStencilAttachment(RenderTarget& rt);

		//private
		namespace impl
		{
			void AddTextureAttachment(RenderTarget& rt, TextureAttachmentType type, s32 filter, s32 wrapMode, u32 layers, s32 mipLevels, bool alpha, bool isHDR);
			void AddDepthAndStencilAttachment(RenderTarget& rt);
			void CreateFrameBufferID(RenderTarget& renderTarget);
			void DestroyFrameBufferID(RenderTarget& renderTarget);
		}

	}

	namespace rt
	{
		template <class ARENA>
		RenderTarget CreateRenderTarget(ARENA& arena, u32 maxNumColorAttachments, u32 maxNumDepthAttachments, u32 maxNumRenderBufferAttachments, u32 xOffset, u32 yOffset, u32 width, u32 height, const char* file, s32 line, const char* description)
		{
			RenderTarget rt;

			if (maxNumColorAttachments > 0)
			{
				rt.colorAttachments = MAKE_SARRAY_VERBOSE(arena, TextureAttachment, maxNumColorAttachments, file, line, description);
			}

			if (maxNumDepthAttachments > 0)
			{
				rt.depthAttachments = MAKE_SARRAY_VERBOSE(arena, TextureAttachment, maxNumDepthAttachments, file, line, description);
			}
			
			if (maxNumRenderBufferAttachments > 0)
			{
				rt.renderBufferAttachments = MAKE_SARRAY_VERBOSE(arena, RenderBufferAttachment, maxNumRenderBufferAttachments, file, line, description);
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
			impl::DestroyFrameBufferID(rt);


			if (rt.renderBufferAttachments)
			{
				FREE(rt.renderBufferAttachments, arena);
			}

			if (rt.colorAttachments)
			{
				FREE(rt.colorAttachments, arena);
			}

			if (rt.depthAttachments)
			{
				FREE(rt.depthAttachments, arena);
			}
		}

	}
}


#endif // ! __RENDER_TARGET_H__
