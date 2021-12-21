#include "r2pch.h"
#include "r2/Render/Renderer/RenderTarget.h"

namespace r2::draw
{
	const u32 RenderTarget::DEFAULT_RENDER_TARGET = 0;

	u64 RenderTarget::MemorySize(u32 numColorAttachments, u32 numDepthAttachments, u32 numRenderBufferAttachments, u64 alignmnet, u32 headerSize, u32 boundsChecking)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::TextureAttachment>::MemorySize(numColorAttachments), alignmnet, headerSize, boundsChecking) +
			   r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::TextureAttachment>::MemorySize(numDepthAttachments), alignmnet, headerSize, boundsChecking) +
			   r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::RenderBufferAttachment>::MemorySize(numRenderBufferAttachments), alignmnet, headerSize, boundsChecking);
	}

	namespace rt
	{
		void AddTextureAttachment(RenderTarget& rt, TextureAttachmentType type, s32 filter, s32 wrapMode, u32 layers, s32 mipLevels, bool alpha, bool isHDR)
		{
			impl::AddTextureAttachment(rt, type, filter, wrapMode, layers, mipLevels, alpha, isHDR);
		}

		void AddDepthAndStencilAttachment(RenderTarget& rt)
		{
			impl::AddDepthAndStencilAttachment(rt);
		}

		/*void SetRenderTarget(const RenderTarget& rt)
		{
			impl::SetDrawBuffers(rt);
			impl::SetViewport(rt);
		}

		void UnsetRenderTarget(const RenderTarget& rt)
		{
			impl::Unbind(rt);
		}*/
	}

	
}