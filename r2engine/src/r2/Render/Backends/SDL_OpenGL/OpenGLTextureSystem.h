#ifndef __OPENGL_TEXTURE_SYSTEM_H__
#define __OPENGL_TEXTURE_SYSTEM_H__

#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Core/Memory/Memory.h"
#include "glad/glad.h"
/*
Implementation of sparse bindless texture arrays from:
https://github.com/nvMcJohn/apitest/blob/master/src/framework/sparse_bindless_texarray.h
*/

namespace r2
{
	template<typename T = GLsizei>
	struct SQueue;
}

namespace r2::draw::tex
{
	
}

namespace r2::draw::gl
{
	namespace tex
	{
		void Commit(r2::draw::tex::TextureHandle& textureHandle);
		void Free(r2::draw::tex::TextureHandle& textureHandle);

		void CompressedTexSubImage2D(const r2::draw::tex::TextureHandle& textureHandle, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
		void TexSubImage2D(const r2::draw::tex::TextureHandle& textureHandle, GLint level, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data);
		void TexSubCubemapImage2D(const r2::draw::tex::TextureHandle& textureHandle, r2::draw::tex::CubemapSide side, GLint level, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data);


		r2::draw::tex::TextureAddress GetAddress(const r2::draw::tex::TextureHandle& textureHandle);

		GLuint GetTexId(const r2::draw::tex::TextureHandle& textureHandle);
	}

	namespace texcontainer
	{
		template<class ARENA>
		r2::draw::tex::TextureContainer* MakeGLTextureContainer(ARENA& arena, GLsizei slices, const r2::draw::tex::TextureFormat& format)
		{
			r2::draw::tex::TextureContainer* container = ALLOC(r2::draw::tex::TextureContainer, arena);

			R2_CHECK(container != nullptr, "We couldn't allocate the texture container!");

			if (container == nullptr)
			{
				return nullptr;
			}

			container->freeSpace = MAKE_SQUEUE(arena, GLsizei, slices);

			bool success = Init(*container, format, slices);

			if (!success)
			{
				FREE(container, arena);
				return nullptr;
			}

			return container;
		}

		template<class ARENA>
		void Destroy(ARENA& arena, r2::draw::tex::TextureContainer* container)
		{
			if (container == nullptr)
				return;

			R2_CHECK(container->freeSpace && r2::squeue::Size(*container->freeSpace) == container->numSlices, "We shouldn't have any allocations in the texture array!");

			if (container->handle != 0) {
				glMakeTextureHandleNonResidentARB(container->handle);
				container->handle = 0;
			}

			glDeleteTextures(1, &container->texId);

			FREE(container->freeSpace, arena);

			FREE(container, arena);
		}

		bool Init(r2::draw::tex::TextureContainer& container, const r2::draw::tex::TextureFormat& format, GLsizei slices);

		GLsizei HasRoom(const r2::draw::tex::TextureContainer& container, u32 numPages);
		GLsizei VirtualAlloc(r2::draw::tex::TextureContainer& container, u32 numPages);
		void VirtualFree(r2::draw::tex::TextureContainer& container, GLsizei slice, u32 numPages);

		void Commit(r2::draw::tex::TextureContainer& container, r2::draw::tex::TextureHandle* _tex);
		void Free(r2::draw::tex::TextureContainer& container, r2::draw::tex::TextureHandle* _tex);



		void CompressedTexSubImage3D(r2::draw::tex::TextureContainer& container, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data);
		void TexSubImage3D(r2::draw::tex::TextureContainer& container, GLint level, GLint xOffset, GLint yOffset, GLint zOffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data);
		void TexSubCubemapImage3D(r2::draw::tex::TextureContainer& container, r2::draw::tex::CubemapSide side, GLint level, GLint xOffset, GLint yOffset, GLint zOffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data);
			
		u64 MemorySize(u64 slices, u64 alignment, u32 headerSize, u32 boundsChecking);

		//Don't use publically 
		void ChangeCommitment(r2::draw::tex::TextureContainer& container, GLsizei slice, GLsizei numLayersToCommit, GLboolean commit);
	}

	namespace texsys
	{
		void MakeNewGLTexture(r2::draw::tex::TextureHandle& handle, const r2::draw::tex::TextureFormat& format, u32 numPages);
		void FreeGLTexture(r2::draw::tex::TextureHandle& handle);
		void AddTexturePages(r2::draw::tex::TextureHandle& handle, u32 numPages);


		//private
		r2::draw::tex::TextureContainer* MakeGLTextureIfNeeded(const r2::draw::tex::TextureFormat& format, u32 slices);
		void AllocGLTexture(r2::draw::tex::TextureHandle& handle, const r2::draw::tex::TextureFormat& format, u32 numPages);
		
	}
}

#endif // __OPENGL_TEXTURE_SYSTEM_H__
