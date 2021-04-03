#ifndef __RENDER_TARGET_H__
#define __RENDER_TARGET_H__

#include "r2/Utils/Utils.h"
#include "r2/Render/Model/Textures/Texture.h"

namespace r2::draw::rt
{
	struct ColorAttachment
	{
		tex::TextureHandle texture;
	};

	struct RenderBufferAttachment
	{
		u32 rbo = 0;
		s32 attachmentType = -1;
	};
}

namespace r2::draw
{
	//@TODO(Serge): implement for real
	struct RenderTarget
	{
		static const u32 DEFAULT_RENDER_TARGET;
		u32 frameBufferID = DEFAULT_RENDER_TARGET;
		u32 width, height;

		r2::SArray<rt::ColorAttachment>* colorAttachments = nullptr;
		r2::SArray<rt::RenderBufferAttachment>* renderBufferAttachments = nullptr;

		static u64 RenderTarget::MemorySize(u64 numColorAttachments, u64 numRenderBufferAttachments, u64 alignmnet, u32 headerSize, u32 boundsChecking);
	};

	namespace rt
	{
		extern s32 COLOR_ATTACHMENT;
		extern s32 DEPTH_ATTACHMENT;
		extern s32 DEPTH_STENCIL_ATTACHMENT;
		extern s32 MSAA_ATTACHMENT;


		template <class ARENA>
		RenderTarget CreateRenderTarget(ARENA& arena, u64 maxNumColorAttachments, u64 maxNumRenderBufferAttachments, u32 width, u32 height, const char* file, s32 line, const char* description);


		template <class ARENA>
		void DestroyRenderTarget(ARENA& arena, RenderTarget& rt);

		void AddTextureAttachment(RenderTarget& rt, s32 filter, s32 wrapMode, s32 mipLevels, bool alpha, bool isHDR);
		void AddDepthAndStencilAttachment(RenderTarget& rt);

		void SetRenderTarget(const RenderTarget& rt);
		void UnsetRenderTarget(const RenderTarget& rt);

		//private
		namespace impl
		{
			void Bind(const RenderTarget& rt);
			void Unbind(const RenderTarget& rt);
			void AddTextureAttachment(RenderTarget& rt, s32 filter, s32 wrapMode, s32 mipLevels, bool alpha, bool isHDR);
			void AddDepthAndStencilAttachment(RenderTarget& rt);
			void CreateFrameBufferID(RenderTarget& renderTarget);
			void DestroyFrameBufferID(RenderTarget& renderTarget);
			void SetDrawBuffers(const RenderTarget& rt);
		}

	}

	namespace rt
	{
		template <class ARENA>
		RenderTarget CreateRenderTarget(ARENA& arena, u64 maxNumColorAttachments, u64 maxNumRenderBufferAttachments, u32 width, u32 height, const char* file, s32 line, const char* description)
		{
			RenderTarget rt;
			rt.colorAttachments = MAKE_SARRAY_VERBOSE(arena, ColorAttachment, maxNumColorAttachments, file, line, description);
			rt.renderBufferAttachments = MAKE_SARRAY_VERBOSE(arena, RenderBufferAttachment, maxNumRenderBufferAttachments, file, line, description);
			rt.width = width;
			rt.height = height;

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
		}

	}
}


#endif // ! __RENDER_TARGET_H__
