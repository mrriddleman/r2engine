#include "r2pch.h"
#include "r2/Render/Renderer/RenderTarget.h"
#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLTextureSystem.h"
#include "glad/glad.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"


namespace r2::draw::rt
{
	s32 COLOR_ATTACHMENT = GL_COLOR_ATTACHMENT0;
	s32 DEPTH_ATTACHMENT = GL_DEPTH_ATTACHMENT;
	s32 DEPTH_STENCIL_ATTACHMENT = GL_STENCIL_ATTACHMENT;
	s32 MSAA_ATTACHMENT = GL_TEXTURE_2D_MULTISAMPLE;
	s32 STENCIL_ATTACHMENT = GL_STENCIL_ATTACHMENT;
}

namespace r2::draw::rt::impl
{

	void AddTextureAttachment(RenderTarget& rt, TextureAttachmentType type, bool swapping, bool uploadAllTextures, s32 filter, s32 wrapMode, u32 layers, s32 mipLevels, bool alpha, bool isHDR, bool useLayeredRenderering, u32 mipLevelToAttach)
	{
		R2_CHECK(layers > 0, "We need at least 1 layer");

		TextureAttachment textureAttachment;

		textureAttachment.type = type;

		tex::TextureFormat format;
		format.width = rt.width;
		format.height = rt.height;
		format.mipLevels = mipLevels;
		textureAttachment.mipLevelAttached = mipLevelToAttach;
		textureAttachment.useLayeredRenderering = useLayeredRenderering;
		
		if (type == COLOR)
		{
			if (isHDR)
			{
				format.internalformat = alpha ? GL_RGBA16F : GL_R11F_G11F_B10F;
			}
			else
			{
				format.internalformat = alpha ? GL_RGBA8 : GL_RGB8;
			}
		}
		else if (type == DEPTH)
		{
			if (isHDR)
			{
				
				format.internalformat = GL_DEPTH_COMPONENT32;
			}
			else
			{
				format.internalformat = GL_DEPTH_COMPONENT24;
			}
			
			format.borderColor = glm::vec4(1.0f);
		}
		else if (type == DEPTH_CUBEMAP)
		{
			if (isHDR)
			{
				format.internalformat = GL_DEPTH_COMPONENT32;
			}
			else
			{
				format.internalformat = GL_DEPTH_COMPONENT16;
			}

			format.borderColor = glm::vec4(1.0f);
			format.isCubemap = true;
		}
		else if (type == RG32F)
		{
			format.internalformat = GL_RG32F;
		}
		else if (type == RG16F)
		{
			format.internalformat = GL_RG16F;
			format.borderColor = glm::vec4(1.0f);
		}
		else if (type == R32F)
		{
			format.internalformat = GL_R32F;
		}
		else if (type == R16F)
		{
			format.internalformat = GL_R16F;
		}
		else if (type == RG16)
		{
			format.internalformat = GL_RG16;
			format.borderColor = glm::vec4(1.0f);
		}
		else if (type == STENCIL8)
		{
			format.internalformat = GL_STENCIL_INDEX8;

		}
		else if (type == DEPTH24_STENCIL8)
		{
			format.internalformat = GL_DEPTH24_STENCIL8;
		}
		else if (type == DEPTH32F_STENCIL8)
		{
			format.internalformat = GL_DEPTH32F_STENCIL8;
		}
		else
		{
			R2_CHECK(false, "Unsupported TextureAttachmentType!");
		}
		
		
		format.minFilter = filter;
		format.magFilter = filter;
		format.wrapMode = wrapMode;

		u32 numTextures = swapping ? MAX_TEXTURE_ATTACHMENT_HISTORY : 1;
		textureAttachment.numTextures = numTextures;
		textureAttachment.uploadAllTextures = uploadAllTextures;
		textureAttachment.format = format;

		bool useMaxPages = true;

		if (format.mipLevels > 1)
		{
			useMaxPages = false;
		}


		for (u32 index = 0; index < numTextures; ++index)
		{
			textureAttachment.texture[index] = tex::CreateTexture(format, layers, useMaxPages);
		}

		u32 currentIndex = textureAttachment.currentTexture;

		glBindFramebuffer(GL_FRAMEBUFFER, rt.frameBufferID);
		if (IsColorAttachment(type))
		{
			if (!useLayeredRenderering)
			{
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + rt.numFrameBufferColorAttachments, textureAttachment.texture[currentIndex].container->texId, mipLevelToAttach, textureAttachment.texture[currentIndex].sliceIndex);
			}
			else
			{
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + rt.numFrameBufferColorAttachments, textureAttachment.texture[currentIndex].container->texId, mipLevelToAttach);
			}

			textureAttachment.colorAttachmentNumber = rt.numFrameBufferColorAttachments;
			rt.numFrameBufferColorAttachments++;
		}
		else if (IsDepthAttachment(type))
		{
			
			if (!useLayeredRenderering)
			{
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureAttachment.texture[currentIndex].container->texId, mipLevelToAttach, textureAttachment.texture[currentIndex].sliceIndex);
			}
			else
			{
				//Have to do layered rendering with this
				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureAttachment.texture[currentIndex].container->texId, mipLevelToAttach);
			}

			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		}
		else if (IsStencilAttachment(type))
		{
			if (!useLayeredRenderering)
			{
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, textureAttachment.texture[currentIndex].container->texId, mipLevelToAttach, textureAttachment.texture[currentIndex].sliceIndex);
			}
			else
			{
				glFramebufferTexture(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, textureAttachment.texture[currentIndex].container->texId, mipLevelToAttach);
			}
		}
		else if (IsDepthStencilAttachment(type))
		{
			if (!useLayeredRenderering)
			{
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, textureAttachment.texture[currentIndex].container->texId, mipLevelToAttach, textureAttachment.texture[currentIndex].sliceIndex);
			}
			else
			{
				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, textureAttachment.texture[currentIndex].container->texId, mipLevelToAttach);
			}
		}
		else
		{
			R2_CHECK(false, "Unknown texture attachment type");
		}

		auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

		R2_CHECK(result, "Failed to attach texture to frame buffer");

		if (IsColorAttachment(type))
		{
			r2::sarr::Push(*rt.colorAttachments, textureAttachment);
		}
		else if (IsDepthAttachment(type))
		{
			r2::sarr::Push(*rt.depthAttachments, textureAttachment);
		}
		else if (IsStencilAttachment(type))
		{
			r2::sarr::Push(*rt.stencilAttachments, textureAttachment);
		}
		else if (IsDepthStencilAttachment(type))
		{
			r2::sarr::Push(*rt.depthStencilAttachments, textureAttachment);
		}
		else
		{
			R2_CHECK(false, "Unknown texture attachment type");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}

	void SetTextureAttachment(RenderTarget& rt, const rt::TextureAttachment& textureAttachment)
	{
		int index = textureAttachment.currentTexture;
		glBindFramebuffer(GL_FRAMEBUFFER, rt.frameBufferID);

		TextureAttachmentType type = textureAttachment.type;

		if (IsDepthAttachment(type))
		{
			if (rt.attachmentReferences && r2::sarr::Size(*rt.attachmentReferences) + 1 <= r2::sarr::Capacity(*rt.attachmentReferences))
			{
				rt::TextureAttachmentReference ref;
				ref.attachmentPtr = &textureAttachment;
				ref.type = type;

				r2::sarr::Push(*rt.attachmentReferences, ref);
			}

			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureAttachment.texture[index].container->texId, textureAttachment.mipLevelAttached, textureAttachment.texture[index].sliceIndex);
			
			auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

			R2_CHECK(result, "Failed to attach texture to frame buffer");
		}
		else if(IsColorAttachment(type))
		{
			if (rt.attachmentReferences && r2::sarr::Size(*rt.attachmentReferences) + 1 <= r2::sarr::Capacity(*rt.attachmentReferences))
			{
				rt::TextureAttachmentReference ref;
				ref.attachmentPtr = &textureAttachment;
				ref.type = type;
				ref.colorAttachmentNumber = rt.numFrameBufferColorAttachments;

				r2::sarr::Push(*rt.attachmentReferences, ref);
			}

			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + rt.numFrameBufferColorAttachments, textureAttachment.texture[index].container->texId, textureAttachment.mipLevelAttached, textureAttachment.texture[index].sliceIndex);

			auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

			R2_CHECK(result, "Failed to attach texture to frame buffer");

			rt.numFrameBufferColorAttachments++;
		}
		else if (IsStencilAttachment(type))
		{
			if (rt.attachmentReferences && r2::sarr::Size(*rt.attachmentReferences) + 1 <= r2::sarr::Capacity(*rt.attachmentReferences))
			{
				rt::TextureAttachmentReference ref;
				ref.attachmentPtr = &textureAttachment;
				ref.type = type;

				r2::sarr::Push(*rt.attachmentReferences, ref);
			}

			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, textureAttachment.texture[index].container->texId, textureAttachment.mipLevelAttached, textureAttachment.texture[index].sliceIndex);

			auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

			R2_CHECK(result, "Failed to attach texture to frame buffer");
		}
		else if (IsDepthStencilAttachment(type))
		{
			if (rt.attachmentReferences && r2::sarr::Size(*rt.attachmentReferences) + 1 <= r2::sarr::Capacity(*rt.attachmentReferences))
			{
				rt::TextureAttachmentReference ref;
				ref.attachmentPtr = &textureAttachment;
				ref.type = type;

				r2::sarr::Push(*rt.attachmentReferences, ref);
			}

			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, textureAttachment.texture[index].container->texId, textureAttachment.mipLevelAttached, textureAttachment.texture[index].sliceIndex);

			auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

			R2_CHECK(result, "Failed to attach texture to frame buffer");
		}
		else
		{
			R2_CHECK(false, "Unsupported texture attachment type: %d", type);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	float AddTexturePagesToAttachment(RenderTarget& rt, TextureAttachmentType type, u32 pages)
	{
		int index = 0;

		r2::SArray<TextureAttachment>* textureAttachmentsToUse = nullptr;

		if (IsColorAttachment(type))
		{
			textureAttachmentsToUse = rt.colorAttachments;
		}
		else if (IsDepthAttachment(type))
		{
			textureAttachmentsToUse = rt.depthAttachments;
		}
		else if (IsStencilAttachment(type))
		{
			textureAttachmentsToUse = rt.stencilAttachments;
		}
		else if (IsDepthStencilAttachment(type))
		{
			textureAttachmentsToUse = rt.depthStencilAttachments;
		}
		else
		{
			R2_CHECK(false, "Unsupported TextureAttachmentType");
		}


		//@NOTE(Serge): we only use the first one right now
		R2_CHECK(r2::sarr::Size(*textureAttachmentsToUse) > 0, "We should have at least one texture here!");


		auto& attachment =  r2::sarr::At(*textureAttachmentsToUse, 0);
		
		R2_CHECK(attachment.numTextures == 1, "Right now we only support adding pages to attachments that aren't swappable");

		r2::draw::tex::TextureHandle textureHandle = attachment.texture[index];

		return tex::AddTexturePages(textureHandle, pages).sliceIndex;
	}

	void RemoveTexturePagesFromAttachment(RenderTarget& rt, TextureAttachmentType type, float index, u32 pages)
	{
		if (!(index >= 0 && pages > 0))
		{
			return;
		}

		r2::SArray<TextureAttachment>* textureAttachmentsToUse = nullptr;

		if (IsColorAttachment(type))
		{
			textureAttachmentsToUse = rt.colorAttachments;
		}
		else if (IsDepthAttachment(type))
		{
			textureAttachmentsToUse = rt.depthAttachments;
		}
		else if (IsStencilAttachment(type))
		{
			textureAttachmentsToUse = rt.stencilAttachments;
		}
		else if (IsDepthStencilAttachment(type))
		{
			textureAttachmentsToUse = rt.depthStencilAttachments;
		}
		else
		{
			R2_CHECK(false, "Unsupported TextureAttachmentType");
		}

		R2_CHECK(r2::sarr::Size(*textureAttachmentsToUse) > 0, "We should have at least one texture here!");

		int textureIndex = 0;
		r2::draw::tex::TextureHandle textureHandle = r2::sarr::At(*textureAttachmentsToUse, 0).texture[textureIndex];
		textureHandle.sliceIndex = index;
		textureHandle.numPages = pages;

		tex::UnloadFromGPU(textureHandle);
	}

	void CreateFrameBufferID(RenderTarget& renderTarget)
	{
		glGenFramebuffers(1, &renderTarget.frameBufferID);
	}

	void DestroyFrameBufferID(RenderTarget& renderTarget)
	{

		if (renderTarget.colorAttachments)
		{
			u64 numColorAttachments = r2::sarr::Size(*renderTarget.colorAttachments);

			for (u64 i = 0; i < numColorAttachments; ++i)
			{
				auto& attachment = r2::sarr::At(*renderTarget.colorAttachments, i);

				for (u32 textureIndex = 0; textureIndex < attachment.numTextures; ++textureIndex)
				{
					tex::UnloadFromGPU(attachment.texture[textureIndex]);
				}
			}
		}

		if (renderTarget.depthAttachments)
		{
			u64 numDepthAttachments = r2::sarr::Size(*renderTarget.depthAttachments);

			for (u64 i = 0; i < numDepthAttachments; ++i)
			{
				auto& attachment = r2::sarr::At(*renderTarget.depthAttachments, i);

				for (u32 textureIndex = 0; textureIndex < attachment.numTextures; ++textureIndex)
				{
					tex::UnloadFromGPU(attachment.texture[textureIndex]);
				}
			}
		}

		if (renderTarget.stencilAttachments)
		{
			u64 numStencilAttachments = r2::sarr::Size(*renderTarget.stencilAttachments);

			for (u64 i = 0; i < numStencilAttachments; ++i)
			{
				auto& attachments = r2::sarr::At(*renderTarget.stencilAttachments, i);

				for (u32 textureIndex = 0; textureIndex < attachments.numTextures; ++textureIndex)
				{
					tex::UnloadFromGPU(attachments.texture[textureIndex]);
				}
			}
		}

		if (renderTarget.depthStencilAttachments)
		{
			u64 numDepthStencilAttachments = r2::sarr::Size(*renderTarget.depthStencilAttachments);

			for (u64 i = 0; i < numDepthStencilAttachments; ++i)
			{
				auto& attachments = r2::sarr::At(*renderTarget.depthStencilAttachments, i);

				for (u32 textureIndex = 0; textureIndex < attachments.numTextures; ++textureIndex)
				{
					tex::UnloadFromGPU(attachments.texture[textureIndex]);
				}
			}
		}
		
		if (renderTarget.renderBufferAttachments)
		{
			u64 numRenderBuffers = r2::sarr::Size(*renderTarget.renderBufferAttachments);
			for (u64 i = 0; i < numRenderBuffers; ++i)
			{
				glDeleteRenderbuffers(1, &r2::sarr::At(*renderTarget.renderBufferAttachments, i).rbo);
			}
		}

		if (renderTarget.frameBufferID > 0)
		{
			glDeleteFramebuffers(1, &renderTarget.frameBufferID);
		}
	}

	void SwapTexturesIfNecessary(RenderTarget& renderTarget)
	{
		if (renderTarget.colorAttachments)
		{
			for (u32 i = 0; i < r2::sarr::Size(*renderTarget.colorAttachments); ++i)
			{
				auto& attachment = r2::sarr::At(*renderTarget.colorAttachments, i);

				if (attachment.numTextures > 1)
				{
					attachment.currentTexture = (attachment.currentTexture + 1) % attachment.numTextures;
					attachment.needsFramebufferUpdate = true;
				}
			}
		}

		if (renderTarget.depthAttachments)
		{
			for (u32 i = 0; i < r2::sarr::Size(*renderTarget.depthAttachments); ++i)
			{
				auto& attachment = r2::sarr::At(*renderTarget.depthAttachments, i);

				if (attachment.numTextures > 1)
				{
					attachment.currentTexture = (attachment.currentTexture + 1) % attachment.numTextures;
					attachment.needsFramebufferUpdate = true;
				}
			}
		}

		if (renderTarget.stencilAttachments)
		{
			for (u32 i = 0; i < r2::sarr::Size(*renderTarget.stencilAttachments); ++i)
			{
				auto& attachment = r2::sarr::At(*renderTarget.stencilAttachments, i);

				if (attachment.numLayers > 1)
				{
					attachment.currentTexture = (attachment.currentTexture + 1) % attachment.numTextures;
					attachment.needsFramebufferUpdate = true;
				}
			}
		}

		if (renderTarget.depthStencilAttachments)
		{
			for (u32 i = 0; i < r2::sarr::Size(*renderTarget.depthStencilAttachments); ++i)
			{
				auto& attachment = r2::sarr::At(*renderTarget.depthStencilAttachments, i);
				if (attachment.numLayers > 1)
				{
					attachment.currentTexture = (attachment.currentTexture + 1) % attachment.numTextures;
					attachment.needsFramebufferUpdate = true;
				}
			}
		}
	}
}