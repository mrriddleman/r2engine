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
			   r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::TextureAttachment>::MemorySize(params.numStencilAttachments), alignmnet, headerSize, boundsChecking) +
			   r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<rt::TextureAttachment>::MemorySize(params.numDepthStencilAttachments), alignmnet, headerSize, boundsChecking) +
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
			cmd.depthTexture = -1;
			cmd.stencilTexture = -1;
			cmd.depthStencilTexture = -1;

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
					if (colorAttachment.textureAttachmentFormat.usesLayeredRenderering)
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
				if (depthAttachment.textureAttachmentFormat.usesLayeredRenderering)
				{
					cmd.depthUseLayeredRenderering = true;
				}
			}

			if (rt.stencilAttachments != nullptr)
			{
				
				const auto numStencilAttachments = r2::sarr::Size(*rt.stencilAttachments);
				if (numStencilAttachments > 0)
				{
					R2_CHECK(numStencilAttachments == 1, "Should only have 1 here!");

					const auto& stencilAttachment = r2::sarr::At(*rt.stencilAttachments, 0);

					cmd.stencilTexture = stencilAttachment.texture[stencilAttachment.currentTexture].container->texId;
					cmd.stencilTextureLayer = stencilAttachment.texture[stencilAttachment.currentTexture].sliceIndex;
					cmd.toStencilMipLevel = mipLevel;

					if (stencilAttachment.textureAttachmentFormat.usesLayeredRenderering)
					{
						cmd.stencilUseLayeredRendering = true;
					}
				}
				
			}

			if (rt.depthStencilAttachments != nullptr)
			{
				const auto numDepthStencilAttachments = r2::sarr::Size(*rt.depthStencilAttachments);
				if (numDepthStencilAttachments > 0)
				{
					R2_CHECK(numDepthStencilAttachments == 1, "Should only have 1 here");

					const auto& depthStencilAttachment = r2::sarr::At(*rt.depthStencilAttachments, 0);

					cmd.depthStencilTexture = depthStencilAttachment.texture[depthStencilAttachment.currentTexture].container->texId;
					cmd.depthStencilTextureLayer = depthStencilAttachment.texture[depthStencilAttachment.currentTexture].sliceIndex;
					cmd.toDepthStencilMipLevel = mipLevel;

					if (depthStencilAttachment.textureAttachmentFormat.usesLayeredRenderering)
					{
						cmd.depthStencilUseLayeredRenderering = true;
					}
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
						if (ref.attachmentPtr->textureAttachmentFormat.usesLayeredRenderering)
						{
							cmd.colorUseLayeredRenderering = true;
						}
					}
					else if(IsDepthAttachment(ref.type))
					{
						cmd.depthTexture = ref.attachmentPtr->texture[ref.attachmentPtr->currentTexture].container->texId;
						cmd.depthTextureLayer = ref.attachmentPtr->texture[ref.attachmentPtr->currentTexture].sliceIndex;
						cmd.toDepthMipLevel = mipLevel;

						//@NOTE: wrong if we want each attachment to use different rendering types (ie. layered vs non-layered)
						if (ref.attachmentPtr->textureAttachmentFormat.usesLayeredRenderering)
						{
							cmd.depthUseLayeredRenderering = true;
						}
					}
					else if (IsStencilAttachment(ref.type))
					{
						cmd.stencilTexture = ref.attachmentPtr->texture[ref.attachmentPtr->currentTexture].container->texId;
						cmd.stencilTextureLayer = ref.attachmentPtr->texture[ref.attachmentPtr->currentTexture].sliceIndex;
						cmd.toStencilMipLevel = mipLevel;

						if (ref.attachmentPtr->textureAttachmentFormat.usesLayeredRenderering)
						{
							cmd.stencilUseLayeredRendering = true;
						}
					}
					else if (IsDepthStencilAttachment(ref.type))
					{
						cmd.depthStencilTexture = ref.attachmentPtr->texture[ref.attachmentPtr->currentTexture].container->texId;
						cmd.depthStencilTextureLayer = ref.attachmentPtr->texture[ref.attachmentPtr->currentTexture].sliceIndex;
						cmd.toDepthStencilMipLevel = mipLevel;

						if (ref.attachmentPtr->textureAttachmentFormat.usesLayeredRenderering)
						{
							cmd.depthStencilUseLayeredRenderering = true;
						}
					}
					else
					{
						R2_CHECK(false, "Unsupported reference type!");
					}
				}
			}
		}

		void FillSetRenderTargetMipLevelCommandWithTextureIndex(const RenderTarget& rt, u32 mipLevel, u32 textureIndex, SetRenderTargetMipLevel& cmd)
		{
			memset(&cmd, 0, sizeof(SetRenderTargetMipLevel));
			
			cmd.depthTexture = -1;
			cmd.stencilTexture = -1;
			cmd.depthStencilTexture = -1;

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
					if (colorAttachment.textureAttachmentFormat.usesLayeredRenderering)
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
				if (depthAttachment.textureAttachmentFormat.usesLayeredRenderering)
				{
					cmd.depthUseLayeredRenderering = true;
				}
			}

			if (rt.stencilAttachments != nullptr)
			{
				const auto numStencilAttachments = r2::sarr::Size(*rt.stencilAttachments);
				R2_CHECK(numStencilAttachments == 1, "Should only have 1 here");

				const auto& stencilAttachment = r2::sarr::At(*rt.stencilAttachments, 0);

				cmd.stencilTexture = stencilAttachment.texture[textureIndex].container->texId;
				cmd.stencilTextureLayer = stencilAttachment.texture[textureIndex].sliceIndex;
				cmd.toStencilMipLevel = mipLevel;

				if (stencilAttachment.textureAttachmentFormat.usesLayeredRenderering)
				{
					cmd.stencilUseLayeredRendering = true;
				}
			}

			if (rt.depthStencilAttachments != nullptr)
			{
				const auto numDepthStencilAttachments = r2::sarr::Size(*rt.depthStencilAttachments);
				if (numDepthStencilAttachments > 0)
				{
					R2_CHECK(numDepthStencilAttachments == 1, "Should only have 1 here");

					const auto& depthStencilAttachment = r2::sarr::At(*rt.depthStencilAttachments, 0);

					cmd.depthStencilTexture = depthStencilAttachment.texture[textureIndex].container->texId;
					cmd.depthStencilTextureLayer = depthStencilAttachment.texture[textureIndex].sliceIndex;
					cmd.toDepthStencilMipLevel = mipLevel;

					if (depthStencilAttachment.textureAttachmentFormat.usesLayeredRenderering)
					{
						cmd.depthStencilUseLayeredRenderering = true;
					}
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
						if (ref.attachmentPtr->textureAttachmentFormat.usesLayeredRenderering)
						{
							cmd.colorUseLayeredRenderering = true;
						}
					}
					else if(IsDepthAttachment(ref.type))
					{
						cmd.depthTexture = ref.attachmentPtr->texture[textureIndex].container->texId;
						cmd.depthTextureLayer = ref.attachmentPtr->texture[textureIndex].sliceIndex;
						cmd.toDepthMipLevel = mipLevel;

						//@NOTE: wrong if we want each attachment to use different rendering types (ie. layered vs non-layered)
						if (ref.attachmentPtr->textureAttachmentFormat.usesLayeredRenderering)
						{
							cmd.depthUseLayeredRenderering = true;
						}
					}
					else if (IsStencilAttachment(ref.type))
					{
						cmd.stencilTexture = ref.attachmentPtr->texture[textureIndex].container->texId;
						cmd.stencilTextureLayer = ref.attachmentPtr->texture[textureIndex].sliceIndex;
						cmd.toStencilMipLevel = mipLevel;

						if (ref.attachmentPtr->textureAttachmentFormat.usesLayeredRenderering)
						{
							cmd.stencilUseLayeredRendering = true;
						}
					}
					else if (IsDepthStencilAttachment(ref.type))
					{
						cmd.depthStencilTexture = ref.attachmentPtr->texture[textureIndex].container->texId;
						cmd.depthStencilTextureLayer = ref.attachmentPtr->texture[textureIndex].sliceIndex;
						cmd.toDepthStencilMipLevel = mipLevel;

						if (ref.attachmentPtr->textureAttachmentFormat.usesLayeredRenderering)
						{
							cmd.depthStencilUseLayeredRenderering = true;
						}
					}
					else
					{
						R2_CHECK(false, "Unsupported reference type!");
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

		void AddTextureAttachment(RenderTarget& rt, const TextureAttachmentFormat& format)
		{
			impl::AddTextureAttachment(rt, format);
		}

		void SetTextureAttachment(RenderTarget& rt, const rt::TextureAttachment& textureAttachment)
		{
			impl::SetTextureAttachment(rt, textureAttachment);
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

		void SwapTexturesIfNecessary(RenderTarget& rt)
		{
			impl::SwapTexturesIfNecessary(rt);
		}

		bool IsColorAttachment(TextureAttachmentType type)
		{
			return type == COLOR || type == RG32F || type == RG16F || type == R32F || type == R16F || type == RG16;
		}

		bool IsDepthAttachment(TextureAttachmentType type)
		{
			return type == DEPTH || type == DEPTH_CUBEMAP;
		}

		bool IsStencilAttachment(TextureAttachmentType type)
		{
			return type == STENCIL8;
		}

		bool IsDepthStencilAttachment(TextureAttachmentType type)
		{
			return type == DEPTH24_STENCIL8 || type == DEPTH32F_STENCIL8;
		}
	}

	
}