#include "r2pch.h"
#include "r2/Render/Renderer/RenderTarget.h"
#include "r2/Render/Renderer/Commands.h"

namespace r2::draw
{
	const u32 RenderTarget::DEFAULT_RENDER_TARGET = 0;

	u64 RenderTarget::MemorySize(const rt::RenderTargetParams& params, u64 alignmnet, u32 headerSize, u32 boundsChecking)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::TextureAttachment>::MemorySize(params.numColorAttachments), alignmnet, headerSize, boundsChecking) +
			   r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::TextureAttachment>::MemorySize(params.numDepthAttachments), alignmnet, headerSize, boundsChecking) +
			   r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::RenderBufferAttachment>::MemorySize(params.numRenderBufferAttachments), alignmnet, headerSize, boundsChecking) +
			   r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::RenderTargetPageAllocation>::MemorySize(params.maxPageAllocations), alignmnet, headerSize, boundsChecking) +
			   r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::TextureAttachmentReference>::MemorySize(params.numAttachmentRefs), alignmnet, headerSize, boundsChecking);
	}


	namespace cmd
	{
		void FillSetRenderTargetMipLevelCommand(const RenderTarget& rt, u32 mipLevel, SetRenderTargetMipLevel& cmd)
		{
			memset(&cmd, 0, sizeof(SetRenderTargetMipLevel));

			cmd.frameBufferID = rt.frameBufferID;
			cmd.xOffset = rt.xOffset;
			cmd.yOffset = rt.yOffset;
			cmd.colorUseLayeredRenderering = false;

			float scale = glm::pow(2.0f, float(mipLevel));
			cmd.width = static_cast<u32>(float(rt.width) / scale);
			cmd.height = static_cast<u32>(float(rt.height) / scale);

			cmd.numColorTextures = 0;

			if (rt.colorAttachments != nullptr)
			{
				cmd.numColorTextures = rt.numFrameBufferColorAttachments;

				const auto numColorAttachments = r2::sarr::Size(*rt.colorAttachments);
				
				for (u32 i = 0; i < numColorAttachments; ++i)
				{
					const auto& colorAttachment = r2::sarr::At(*rt.colorAttachments, i);

					cmd.colorTextures[colorAttachment.colorAttachmentNumber] = colorAttachment.texture[colorAttachment.currentTexture].container->texId;
					cmd.colorTextureLayers[colorAttachment.colorAttachmentNumber] = colorAttachment.texture[colorAttachment.currentTexture].sliceIndex;
					cmd.toColorMipLevels[colorAttachment.colorAttachmentNumber] = mipLevel;

					//@NOTE: wrong if we want each attachment to use different rendering types (ie. layered vs non-layered)
					if (colorAttachment.useLayeredRenderering)
					{
						cmd.colorUseLayeredRenderering = true;
					}
				}
			}
			
			if (rt.depthAttachments != nullptr)
			{
				const auto numDepthAttachments = r2::sarr::Size(*rt.depthAttachments);
				R2_CHECK(numDepthAttachments == 1, "Should only have 1 here!");

				const auto& depthAttachment = r2::sarr::At(*rt.depthAttachments, 0);

				cmd.depthTexture = depthAttachment.texture[depthAttachment.currentTexture].container->texId;
				cmd.depthTextureLayer = depthAttachment.texture[depthAttachment.currentTexture].sliceIndex;
				cmd.toDepthMipLevel = mipLevel;

				//@NOTE: wrong if we want each attachment to use different rendering types (ie. layered vs non-layered)
				if (depthAttachment.useLayeredRenderering)
				{
					cmd.depthUseLayeredRenderering = true;
				}
			}

			if (rt.attachmentReferences != nullptr)
			{
				const auto numReferences = r2::sarr::Size(*rt.attachmentReferences);

				for (u32 i = 0; i < numReferences; ++i)
				{
					const auto& ref = r2::sarr::At(*rt.attachmentReferences, i);

					if (IsColorAttachment(ref.type))
					{
						cmd.colorTextures[ref.colorAttachmentNumber] = ref.attachmentPtr->texture[ref.attachmentPtr->currentTexture].container->texId;
						cmd.colorTextureLayers[ref.colorAttachmentNumber] = ref.attachmentPtr->texture[ref.attachmentPtr->currentTexture].sliceIndex;
						cmd.toColorMipLevels[ref.colorAttachmentNumber] = mipLevel;

						//@NOTE: wrong if we want each attachment to use different rendering types (ie. layered vs non-layered)
						if (ref.attachmentPtr->useLayeredRenderering)
						{
							cmd.colorUseLayeredRenderering = true;
						}
					}
					else
					{
						cmd.depthTexture = ref.attachmentPtr->texture[ref.attachmentPtr->currentTexture].container->texId;
						cmd.depthTextureLayer = ref.attachmentPtr->texture[ref.attachmentPtr->currentTexture].sliceIndex;
						cmd.toDepthMipLevel = mipLevel;

						//@NOTE: wrong if we want each attachment to use different rendering types (ie. layered vs non-layered)
						if (ref.attachmentPtr->useLayeredRenderering)
						{
							cmd.depthUseLayeredRenderering = true;
						}
					}
				}
			}
		}

		void FillSetRenderTargetMipLevelCommandWithTextureIndex(const RenderTarget& rt, u32 mipLevel, u32 textureIndex, SetRenderTargetMipLevel& cmd)
		{
			memset(&cmd, 0, sizeof(SetRenderTargetMipLevel));

			cmd.frameBufferID = rt.frameBufferID;
			cmd.xOffset = rt.xOffset;
			cmd.yOffset = rt.yOffset;
			cmd.colorUseLayeredRenderering = false;

			float scale = glm::pow(2.0f, float(mipLevel));
			cmd.width = static_cast<u32>(float(rt.width) / scale);
			cmd.height = static_cast<u32>(float(rt.height) / scale);

			cmd.numColorTextures = 0;

			if (rt.colorAttachments != nullptr)
			{
				cmd.numColorTextures = rt.numFrameBufferColorAttachments;

				const auto numColorAttachments = r2::sarr::Size(*rt.colorAttachments);

				R2_CHECK(numColorAttachments == 1, "Should be 1");

				for (u32 i = 0; i < numColorAttachments; ++i)
				{
					const auto& colorAttachment = r2::sarr::At(*rt.colorAttachments, i);

					cmd.colorTextures[i] = colorAttachment.texture[textureIndex].container->texId;
					cmd.colorTextureLayers[i] = colorAttachment.texture[textureIndex].sliceIndex;
					cmd.toColorMipLevels[i] = mipLevel;

					//@NOTE: wrong if we want each attachment to use different rendering types (ie. layered vs non-layered)
					if (colorAttachment.useLayeredRenderering)
					{
						cmd.colorUseLayeredRenderering = true;
					}
				}
			}

			if (rt.depthAttachments != nullptr)
			{
				const auto numDepthAttachments = r2::sarr::Size(*rt.depthAttachments);
				R2_CHECK(numDepthAttachments == 1, "Should only have 1 here!");

				const auto& depthAttachment = r2::sarr::At(*rt.depthAttachments, 0);

				cmd.depthTexture = depthAttachment.texture[textureIndex].container->texId;
				cmd.depthTextureLayer = depthAttachment.texture[textureIndex].sliceIndex;
				cmd.toDepthMipLevel = mipLevel;

				//@NOTE: wrong if we want each attachment to use different rendering types (ie. layered vs non-layered)
				if (depthAttachment.useLayeredRenderering)
				{
					cmd.depthUseLayeredRenderering = true;
				}
			}

			if (rt.attachmentReferences != nullptr)
			{
				const auto numReferences = r2::sarr::Size(*rt.attachmentReferences);

				R2_CHECK(numReferences == 1, "Should be 1");

				for (u32 i = 0; i < numReferences; ++i)
				{
					const auto& ref = r2::sarr::At(*rt.attachmentReferences, i);

					if (IsColorAttachment(ref.type))
					{
						cmd.colorTextures[textureIndex] = ref.attachmentPtr->texture[textureIndex].container->texId;
						cmd.colorTextureLayers[textureIndex] = ref.attachmentPtr->texture[textureIndex].sliceIndex;
						cmd.toColorMipLevels[textureIndex] = mipLevel;

						//@NOTE: wrong if we want each attachment to use different rendering types (ie. layered vs non-layered)
						if (ref.attachmentPtr->useLayeredRenderering)
						{
							cmd.colorUseLayeredRenderering = true;
						}
					}
					else
					{
						cmd.depthTexture = ref.attachmentPtr->texture[textureIndex].container->texId;
						cmd.depthTextureLayer = ref.attachmentPtr->texture[textureIndex].sliceIndex;
						cmd.toDepthMipLevel = mipLevel;

						//@NOTE: wrong if we want each attachment to use different rendering types (ie. layered vs non-layered)
						if (ref.attachmentPtr->useLayeredRenderering)
						{
							cmd.depthUseLayeredRenderering = true;
						}
					}
				}
			}
		}
	}

	namespace rt
	{
		bool IsPageAllocationEqual(const RenderTargetPageAllocation& page1, const RenderTargetPageAllocation& page2)
		{
			return page1.attachmentIndex == page2.attachmentIndex && page1.numPages == page2.numPages && page1.sliceIndex == page2.sliceIndex && page1.type == page2.type;
		}

		void AddTextureAttachment(RenderTarget& rt, TextureAttachmentType type, s32 filter, s32 wrapMode, u32 layers, s32 mipLevels, bool alpha, bool isHDR, bool useLayeredRendering)
		{
			impl::AddTextureAttachment(rt, type, false, false, filter, wrapMode, layers, mipLevels, alpha, isHDR, useLayeredRendering, 0);
		}

		void AddTextureAttachment(RenderTarget& rt, TextureAttachmentType type, bool swapping, bool uploadAllTextures, s32 filter, s32 wrapMode, u32 layers, s32 mipLevels, bool alpha, bool isHDR, bool useLayeredRendering, u32 mipLevelToAttach)
		{
			impl::AddTextureAttachment(rt, type, swapping, uploadAllTextures, filter, wrapMode, layers, mipLevels, alpha, isHDR, useLayeredRendering, mipLevelToAttach);
		}

		void SetTextureAttachment(RenderTarget& rt, TextureAttachmentType type, const rt::TextureAttachment& textureAttachment)
		{
			impl::SetTextureAttachment(rt, type, textureAttachment);
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

		void SwapTexturesIfNecessary(RenderTarget& rt)
		{
			impl::SwapTexturesIfNecessary(rt);
		}

		//void UpdateRenderTargetIfNecessary(RenderTarget& rt)
		//{
		//	impl::UpdateRenderTargetIfNecessary(rt);
		//}

		bool IsColorAttachment(TextureAttachmentType type)
		{
			return type == COLOR || type == RG32F || type == RG16F || type == R32F || type == R16F || type == RG16;
		}

		bool IsDepthAttachment(TextureAttachmentType type)
		{
			return type == DEPTH || type == DEPTH_CUBEMAP;
		}
	}

	
}