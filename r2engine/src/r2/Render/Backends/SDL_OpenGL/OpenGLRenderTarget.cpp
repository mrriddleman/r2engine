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
	/*void Bind(const RenderTarget& rt)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, rt.frameBufferID);
	}
	
	void Unbind(const RenderTarget& rt)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void SetDrawBuffers(const RenderTarget& rt)
	{
		Bind(rt);

		if (rt.colorAttachments && rt.frameBufferID != 0)
		{
			const u64 numBufs = r2::sarr::Size(*rt.colorAttachments);
			GLenum* bufs = ALLOC_ARRAYN(GLenum, numBufs, *MEM_ENG_SCRATCH_PTR);

			for (u64 i = 0; i < numBufs; ++i)
			{
				bufs[i] = GL_COLOR_ATTACHMENT0 + i;
			}

			glDrawBuffers(numBufs, bufs);

			FREE_ARRAY(bufs, *MEM_ENG_SCRATCH_PTR);
		}

	}

	void SetViewport(const RenderTarget& renderTarget)
	{
		glViewport(renderTarget.xOffset, renderTarget.yOffset, renderTarget.width, renderTarget.height);
	}*/

	void AddTextureAttachment(RenderTarget& rt, TextureAttachmentType type, s32 filter, s32 wrapMode, u32 layers, s32 mipLevels, bool alpha, bool isHDR)
	{
		R2_CHECK(layers > 0, "We need at least 1 layer");

		//@TODO(Serge): implement mip map levels?

		TextureAttachment textureAttachment;

		tex::TextureFormat format;
		format.width = rt.width;
		format.height = rt.height;
		format.mipLevels = mipLevels;
		format.numCommitLayers = layers;

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
		else
		{
			R2_CHECK(false, "Unsupported TextureAttachmentType!");
		}
		
		
		format.minFilter = filter;
		format.magFilter = filter;
		format.wrapMode = wrapMode;

		textureAttachment.texture = tex::CreateTexture(format);

		glBindFramebuffer(GL_FRAMEBUFFER, rt.frameBufferID);
		if (type == COLOR)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLenum)r2::sarr::Size(*rt.colorAttachments), textureAttachment.texture.container->texId, 0, textureAttachment.texture.sliceIndex);
		}
		else if (type == DEPTH)
		{
			//Have to do layered rendering with this
			if (layers == 1)
			{
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureAttachment.texture.container->texId, 0, textureAttachment.texture.sliceIndex);
			}
			else
			{
				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureAttachment.texture.container->texId, 0);
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

		if (type == COLOR)
		{
			r2::sarr::Push(*rt.colorAttachments, textureAttachment);
		}
		else if (type == DEPTH)
		{
			r2::sarr::Push(*rt.depthAttachments, textureAttachment);
		}
		else
		{
			R2_CHECK(false, "Unknown texture attachment type");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
				tex::UnloadFromGPU(r2::sarr::At(*renderTarget.colorAttachments, i).texture);
			}
		}

		if (renderTarget.depthAttachments)
		{
			u64 numDepthAttachments = r2::sarr::Size(*renderTarget.depthAttachments);

			for (u64 i = 0; i < numDepthAttachments; ++i)
			{
				tex::UnloadFromGPU(r2::sarr::At(*renderTarget.depthAttachments, i).texture);
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
}