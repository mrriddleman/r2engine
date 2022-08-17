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
}

namespace r2::draw::rt::impl
{
	void AddTextureAttachment(RenderTarget& rt, TextureAttachmentType type, bool swapping, bool uploadAllTextures, s32 filter, s32 wrapMode, u32 layers, s32 mipLevels, bool alpha, bool isHDR, bool useLayeredRenderering)
	{
		R2_CHECK(layers > 0, "We need at least 1 layer");

		TextureAttachment textureAttachment;

		tex::TextureFormat format;
		format.width = rt.width;
		format.height = rt.height;
		format.mipLevels = mipLevels;

		if (type == COLOR)
		{
			if (isHDR)
			{
				format.internalformat = alpha ? GL_RGBA16F : GL_RGB16F;
			}
			else
			{
				format.internalformat = alpha ? GL_RGBA8 : GL_RGB8;
			}
		}
		else if (type == DEPTH)
		{
			format.internalformat = GL_DEPTH_COMPONENT16;
			format.borderColor = glm::vec4(1.0f);
		}
		else if (type == DEPTH_CUBEMAP)
		{
			format.internalformat = GL_DEPTH_COMPONENT16;
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

		for (u32 index = 0; index < numTextures; ++index)
		{
			textureAttachment.texture[index] = tex::CreateTexture(format, layers);
		}

		u32 currentIndex = textureAttachment.currentTexture;

		glBindFramebuffer(GL_FRAMEBUFFER, rt.frameBufferID);
		if (type == COLOR || type == RG32F || type == RG16F)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLenum)r2::sarr::Size(*rt.colorAttachments), textureAttachment.texture[currentIndex].container->texId, 0, textureAttachment.texture[currentIndex].sliceIndex);
		}
		else if (type == DEPTH || type == DEPTH_CUBEMAP)
		{
			
			if (!useLayeredRenderering)
			{
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureAttachment.texture[currentIndex].container->texId, 0, textureAttachment.texture[currentIndex].sliceIndex);
			}
			else
			{
				//Have to do layered rendering with this
				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureAttachment.texture[currentIndex].container->texId, 0);
			}

			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		}
		else
		{
			R2_CHECK(false, "Unknown texture attachment type");
		}

		auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

		R2_CHECK(result, "Failed to attach texture to frame buffer");

		if (type == COLOR || type == RG32F || type == RG16F)
		{
			r2::sarr::Push(*rt.colorAttachments, textureAttachment);
		}
		else if (type == DEPTH || type == DEPTH_CUBEMAP)
		{
			r2::sarr::Push(*rt.depthAttachments, textureAttachment);
		}
		else
		{
			R2_CHECK(false, "Unknown texture attachment type");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}

	void SetTextureAttachment(RenderTarget& rt, TextureAttachmentType type, const rt::TextureAttachment& textureAttachment)
	{
		int index = textureAttachment.currentTexture;
		glBindFramebuffer(GL_FRAMEBUFFER, rt.frameBufferID);

		if (type == DEPTH)
		{
			if (rt.attachmentReferences && r2::sarr::Size(*rt.attachmentReferences) + 1 <= r2::sarr::Capacity(*rt.attachmentReferences))
			{
				rt::TextureAttachmentReference ref;
				ref.attachmentPtr = &textureAttachment;
				ref.type = type;

				r2::sarr::Push(*rt.attachmentReferences, ref);
			}

			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureAttachment.texture[index].container->texId, 0, textureAttachment.texture[index].sliceIndex);
			
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

		if (type == COLOR)
		{
			textureAttachmentsToUse = rt.colorAttachments;
		}
		else if (type == DEPTH || type == DEPTH_CUBEMAP)
		{
			textureAttachmentsToUse = rt.depthAttachments;
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

		if (type == COLOR)
		{
			textureAttachmentsToUse = rt.colorAttachments;
		}
		else if (type == DEPTH || type == DEPTH_CUBEMAP)
		{
			textureAttachmentsToUse = rt.depthAttachments;
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

	void AddDepthAndStencilAttachment(RenderTarget& rt)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, rt.frameBufferID);
		u32 rbo;
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, rt.width, rt.height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

		auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

		R2_CHECK(result, "Failed to attach texture to frame buffer");

		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		RenderBufferAttachment ra;
		ra.rbo = rbo;
		ra.attachmentType = DEPTH_STENCIL_ATTACHMENT;

		r2::sarr::Push(*rt.renderBufferAttachments, ra);
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
					R2_CHECK(attachment.currentTexture != 2, "here");
					attachment.needsFramebufferUpdate = true;
				}
			}
		}
	}

	void UpdateRenderTargetIfNecessary(RenderTarget& renderTarget)
	{
		if (renderTarget.colorAttachments)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, renderTarget.frameBufferID);

			const auto numColorAttachments = r2::sarr::Size(*renderTarget.colorAttachments);

			for (u64 i = 0; i < numColorAttachments; ++i)
			{
				const auto& attachment = r2::sarr::At(*renderTarget.colorAttachments, i);
				u32 currentIndex = attachment.currentTexture;

				if (attachment.numTextures > 1 && attachment.needsFramebufferUpdate)
				{
					glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, attachment.texture[currentIndex].container->texId, 0, attachment.texture[currentIndex].sliceIndex);
				
				
					auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

					R2_CHECK(result, "Failed to attach texture to frame buffer");
				
				}
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		if (renderTarget.depthAttachments)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, renderTarget.frameBufferID);

			const auto numDepthAttachments = r2::sarr::Size(*renderTarget.depthAttachments);

			for (u64 i = 0; i < numDepthAttachments; ++i)
			{
				const auto& attachment = r2::sarr::At(*renderTarget.depthAttachments, i);

				if (attachment.numTextures > 1 && attachment.needsFramebufferUpdate)
				{
					if (attachment.numLayers == 1)
					{
						glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, attachment.texture[attachment.currentTexture].container->texId, 0, attachment.texture[attachment.currentTexture].sliceIndex);
					}
					else
					{
						//Have to do layered rendering with this
						glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, attachment.texture[attachment.currentTexture].container->texId, 0);
					}

					glDrawBuffer(GL_NONE);
					glReadBuffer(GL_NONE);

					auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

					R2_CHECK(result, "Failed to attach texture to frame buffer");
				}
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		if (renderTarget.attachmentReferences)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, renderTarget.frameBufferID);

			const auto numReferences = r2::sarr::Size(*renderTarget.attachmentReferences);

			for (u64 i = 0; i < numReferences; ++i)
			{
				 const auto& attachmentReference = r2::sarr::At(*renderTarget.attachmentReferences, i);

				 R2_CHECK(attachmentReference.attachmentPtr != nullptr, "attachment ptr is nullptr!");

				 if (attachmentReference.type == DEPTH)
				 {
					 if (attachmentReference.attachmentPtr->numTextures > 1 && attachmentReference.attachmentPtr->needsFramebufferUpdate)
					 {
						 const auto currentTexture = attachmentReference.attachmentPtr->currentTexture;
						 const auto* textureAttachmentPtr = attachmentReference.attachmentPtr;

						 glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureAttachmentPtr->texture[currentTexture].container->texId, 0, textureAttachmentPtr->texture[currentTexture].sliceIndex);

						 auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

						 R2_CHECK(result, "Failed to attach texture to frame buffer");
					 }
				 }
				 else
				 {
					 R2_CHECK(false, "Unsupported attachment reference type");
				 }
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}
}