#include "r2pch.h"
#include "r2/Render/Renderer/RenderTarget.h"

namespace r2::draw
{
	const u32 RenderTarget::DEFAULT_RENDER_TARGET = 0;

	u64 RenderTarget::MemorySize(u64 numColorAttachments, u64 numRenderBufferAttachments, u64 alignmnet, u32 headerSize, u32 boundsChecking)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::ColorAttachment>::MemorySize(numColorAttachments), alignmnet, headerSize, boundsChecking) +
			   r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::RenderBufferAttachment>::MemorySize(numRenderBufferAttachments), alignmnet, headerSize, boundsChecking);
	}

	namespace rt
	{
		void AddTextureAttachment(RenderTarget& rt, s32 filter, s32 wrapMode, s32 mipLevels, bool alpha)
		{
			impl::AddTextureAttachment(rt, filter, wrapMode, mipLevels, alpha);
		}

		void AddDepthAndStencilAttachment(RenderTarget& rt)
		{
			impl::AddDepthAndStencilAttachment(rt);
		}

		void SetRenderTarget(const RenderTarget& rt)
		{
			impl::SetDrawBuffers(rt);
		}

		void UnsetRenderTarget(const RenderTarget& rt)
		{
			impl::Unbind(rt);
		}
	}

	
}