#include "r2pch.h"
#include "r2/Render/Renderer/RenderTarget.h"

namespace r2::draw
{
	const u32 RenderTarget::DEFAULT_RENDER_TARGET = 0;

	u64 RenderTarget::MemorySize(u32 numColorAttachments, u32 numDepthAttachments, u32 numRenderBufferAttachments, u32 maxPageAllocations, u64 alignmnet, u32 headerSize, u32 boundsChecking)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::TextureAttachment>::MemorySize(numColorAttachments), alignmnet, headerSize, boundsChecking) +
			   r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::TextureAttachment>::MemorySize(numDepthAttachments), alignmnet, headerSize, boundsChecking) +
			   r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::RenderBufferAttachment>::MemorySize(numRenderBufferAttachments), alignmnet, headerSize, boundsChecking) +
			   r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::RenderTargetPageAllocation>::MemorySize(maxPageAllocations), alignmnet, headerSize, boundsChecking);
	}

	namespace rt
	{
		bool IsPageAllocationEqual(const RenderTargetPageAllocation& page1, const RenderTargetPageAllocation& page2)
		{
			return page1.attachmentIndex == page2.attachmentIndex && page1.numPages == page2.numPages && page1.sliceIndex == page2.sliceIndex && page1.type == page2.type;
		}


		void AddTextureAttachment(RenderTarget& rt, TextureAttachmentType type, s32 filter, s32 wrapMode, u32 layers, s32 mipLevels, bool alpha, bool isHDR, bool useLayeredRendering)
		{
			impl::AddTextureAttachment(rt, type, filter, wrapMode, layers, mipLevels, alpha, isHDR, useLayeredRendering);
		}

		float AddTexturePagesToAttachment(RenderTarget& rt, TextureAttachmentType type, u32 pages)
		{
			float sliceIndex = impl::AddTexturePagesToAttachment(rt, type, pages);

			RenderTargetPageAllocation newAllocation;
			newAllocation.attachmentIndex = 0;
			newAllocation.numPages = pages;
			newAllocation.sliceIndex = sliceIndex;
			newAllocation.type = type;

			r2::sarr::Push(*rt.pageAllocations, newAllocation);


			return sliceIndex;
		}

		void RemoveTexturePagesFromAttachment(RenderTarget& rt, TextureAttachmentType type, float index, u32 pages)
		{
			impl::RemoveTexturePagesFromAttachment(rt, type, index, pages);

			const auto numAllocations = r2::sarr::Size(*rt.pageAllocations);

			RenderTargetPageAllocation allocationToRemove;
			allocationToRemove.attachmentIndex = 0;
			allocationToRemove.numPages = pages;
			allocationToRemove.sliceIndex = index;
			allocationToRemove.type = type;

			for (u32 i = 0; i < numAllocations; ++i)
			{
				const RenderTargetPageAllocation& allocation = r2::sarr::At(*rt.pageAllocations, i);

				if (IsPageAllocationEqual(allocationToRemove, allocation))
				{
					r2::sarr::RemoveAndSwapWithLastElement(*rt.pageAllocations, i);
					break;
				}
			}

		}

		void AddDepthAndStencilAttachment(RenderTarget& rt)
		{
			impl::AddDepthAndStencilAttachment(rt);
		}
	}

	
}