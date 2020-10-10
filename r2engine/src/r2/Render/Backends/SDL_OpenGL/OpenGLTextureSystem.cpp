#include "r2pch.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLTextureSystem.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLUtils.h"


namespace
{
	struct GLTextureSystem
	{
		r2::mem::utils::MemBoundary boundary = {};
		r2::mem::LinearArena* arena = nullptr;
		r2::SHashMap<r2::SArray<r2::draw::tex::TextureContainer*>*>* texArray2Ds = nullptr;
		u32 numTextureContainersPerFormat = 0;
		u32 maxNumTextureContainerLayers = 0;
		b32 sparse = true;
	};

	static GLTextureSystem* s_glTextureSystem = nullptr;

	/* FORMAT KEY
		32 bits          14       14       4
	+-----------------+-------+--------+------+
	| Internal Format | Width | Height | Mips |
	+-----------------+-------+--------+------+
	*/

	enum : u64
	{
		FORMAT_BITS_TOTAL = 0x40ull,

		BITS_INTERNAL_FORMAT = 0x20ull,
		BITS_WIDTH = 0xEull,
		BITS_HEIGHT = 0xEull,
		BITS_MIPS = 0x4ull,

		KEY_INTERNAL_FORMAT_OFFSET = FORMAT_BITS_TOTAL - BITS_INTERNAL_FORMAT,
		KEY_WIDTH_OFFSET = KEY_INTERNAL_FORMAT_OFFSET - BITS_WIDTH,
		KEY_HEIGHT_OFFSET = KEY_WIDTH_OFFSET - BITS_HEIGHT,
		KEY_MIPS_OFFSET = KEY_HEIGHT_OFFSET - BITS_MIPS,
	};

	u64 ConvertFormat(const r2::draw::tex::TextureFormat& format)
	{
		u64 key = 0;
		key |= ENCODE_KEY_VALUE((u64)format.internalformat, BITS_INTERNAL_FORMAT, KEY_INTERNAL_FORMAT_OFFSET);
		key |= ENCODE_KEY_VALUE((u64)format.width, BITS_WIDTH, KEY_WIDTH_OFFSET);
		key |= ENCODE_KEY_VALUE((u64)format.height, BITS_HEIGHT, KEY_HEIGHT_OFFSET);
		key |= ENCODE_KEY_VALUE((u64)format.mipLevels, BITS_MIPS, KEY_MIPS_OFFSET);

		return key;
	}
}


namespace r2::draw::gl
{
	namespace tex
	{
		void Commit(r2::draw::tex::TextureHandle& textureHandle)
		{
			texcontainer::Commit(*textureHandle.container, &textureHandle);
		}

		void Free(r2::draw::tex::TextureHandle& textureHandle)
		{
			texcontainer::Free(*textureHandle.container, &textureHandle);
 		}

		void CompressedTexSubImage2D(const r2::draw::tex::TextureHandle& textureHandle, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data)
		{
			texcontainer::CompressedTexSubImage3D(*textureHandle.container, level, xoffset, yoffset, (GLint)textureHandle.sliceIndex, width, height, 1, format, imageSize, data);
		}

		void TexSubImage2D(const r2::draw::tex::TextureHandle& textureHandle, GLint level, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data)
		{
			texcontainer::TexSubImage3D(*textureHandle.container, level, xOffset, yOffset, (GLint)textureHandle.sliceIndex, width, height, 1, format, type, data);
		}

		r2::draw::tex::TextureAddress GetAddress(const r2::draw::tex::TextureHandle& textureHandle)
		{
			r2::draw::tex::TextureAddress addr{textureHandle.container->handle, textureHandle.sliceIndex};
			return addr;
		}

		GLuint GetTexId(const r2::draw::tex::TextureHandle& textureHandle)
		{
			return textureHandle.container->texId;
		}
	}

	namespace texcontainer
	{
		bool Init(r2::draw::tex::TextureContainer& container, const r2::draw::tex::TextureFormat& format, GLsizei slices, bool sparse)
		{
			container.format = format;
			container.numSlices = slices;
			container.isSparse = sparse;

			GLCall( glGenTextures(1, &container.texId) );
			GLCall( glBindTexture(GL_TEXTURE_2D_ARRAY, container.texId) );

			if (sparse)
			{
				GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SPARSE_ARB, GL_TRUE));

				//@TODO(Serge): pull from format?
				GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
				GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
				GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
				GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));


				// TODO: This could be done once per internal format. For now, just do it every time.
				GLint indexCount = 0,
					xSize = 0,
					ySize = 0,
					zSize = 0;

				GLint bestIndex = -1,
					bestXSize = 0,
					bestYSize = 0;

				GLCall(glGetInternalformativ(GL_TEXTURE_2D_ARRAY, format.internalformat, GL_NUM_VIRTUAL_PAGE_SIZES_ARB, 1, &indexCount));
				
				for (GLint i = 0; i < indexCount; ++i) {
					GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_VIRTUAL_PAGE_SIZE_INDEX_ARB, i));
					GLCall(glGetInternalformativ(GL_TEXTURE_2D_ARRAY, format.internalformat, GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &xSize));
					GLCall(glGetInternalformativ(GL_TEXTURE_2D_ARRAY, format.internalformat, GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &ySize));
					GLCall(glGetInternalformativ(GL_TEXTURE_2D_ARRAY, format.internalformat, GL_VIRTUAL_PAGE_SIZE_Z_ARB, 1, &zSize));

					// For our purposes, the "best" format is the one that winds up with Z=1 and the largest x and y sizes.
					if (zSize == 1) {
						if (xSize >= bestXSize && ySize >= bestYSize) {
							bestIndex = i;
							bestXSize = xSize;
							bestYSize = ySize;
						}
					}
				}

				// This would mean the implementation has no valid sizes for us, or that this format doesn't actually support sparse
				// texture allocation. Need to implement the fallback. TODO: Implement that.
				R2_CHECK(bestIndex != -1, "Implementation has no valid sizes for us!");
				if (bestIndex == -1)
				{
					GLCall(glDeleteTextures(1, &container.texId));
					return false;
				}
					
				container.xTileSize = bestXSize;
				container.yTileSize = bestYSize;

				GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_VIRTUAL_PAGE_SIZE_INDEX_ARB, bestIndex));
			}

			GLCall(glTexStorage3D(GL_TEXTURE_2D_ARRAY, format.mipLevels, format.internalformat, format.width, format.height, slices));

			if (!container.freeSpace ||  r2::squeue::Space(*container.freeSpace) < slices)
			{
				GLCall(glDeleteTextures(1, &container.texId));
				return false;
			}
			
			for (GLsizei i = 0; i < slices; ++i) 
			{
				r2::squeue::PushBack(*container.freeSpace, i);
			}

			if (sparse) {
				container.handle = glGetTextureHandleARB(container.texId);
				if (GLenum err = glGetError())
				{
					R2_CHECK(false, "");
				}
				R2_CHECK(container.handle != 0, "We couldn't get a proper handle to the texture array!");
				GLCall(glMakeTextureHandleResidentARB(container.handle));


			}

			return true;
		}

		GLsizei HasRoom(const r2::draw::tex::TextureContainer& container)
		{
			return r2::squeue::Size(*container.freeSpace) > 0;
		}

		GLsizei VirtualAlloc(r2::draw::tex::TextureContainer& container)
		{
			GLsizei front = r2::squeue::First(*container.freeSpace);
			r2::squeue::PopFront(*container.freeSpace);
			return front;
		}

		void VirtualFree(r2::draw::tex::TextureContainer& container, GLsizei slice)
		{
			r2::squeue::PushBack(*container.freeSpace, slice);
		}

		void Commit(r2::draw::tex::TextureContainer& container, r2::draw::tex::TextureHandle* tex)
		{
			R2_CHECK(tex && tex->container == &container, "You're trying to commit a texture that doesn't belong in this container!");
			if (!tex || tex->container != &container)
			{
				R2_CHECK(false, "Trying to commit a null texture or to a container that doesn't match!");
				return;
			}

			ChangeCommitment(container, (GLsizei)tex->sliceIndex, GL_TRUE);
		}

		void Free(r2::draw::tex::TextureContainer& container, r2::draw::tex::TextureHandle* tex)
		{
			R2_CHECK(tex && tex->container == &container, "You're trying to free a texture that doesn't belong in this container!");
			if (!tex || tex->container != &container)
			{
				R2_CHECK(false, "Trying to free a null texture or to a container that doesn't match!");
				return;
			}

			ChangeCommitment(container, (GLsizei)tex->sliceIndex, GL_FALSE);
		}

		void CompressedTexSubImage3D(r2::draw::tex::TextureContainer& container, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data)
		{
			GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, container.texId));
			GLCall(glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data));
		}

		void TexSubImage3D(r2::draw::tex::TextureContainer& constainer, GLint level, GLint xOffset, GLint yOffset, GLint zOffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data)
		{
			GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, constainer.texId));
			GLCall(glTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, xOffset, yOffset, zOffset, width, height, depth, format, type, data));

			auto err = glGetError();
			if (err != 0)
			{
				R2_CHECK(false, "Failed to sub texture ");
			}
		}

		u64 MemorySize(u64 slices, u64 alignment, u32 headerSize, u32 boundsChecking)
		{
			return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::draw::tex::TextureContainer), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SQueue<GLsizei>::MemorySize(slices), alignment, headerSize, boundsChecking);
		}

		//Don't use publically 
		void ChangeCommitment(r2::draw::tex::TextureContainer& container, GLsizei slice, GLboolean commit)
		{
			GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, container.texId));

			GLsizei levelWidth = container.format.width,
				levelHeight = container.format.height;

			for (int level = 0; level < container.format.mipLevels; ++level) {
				GLCall(glTexPageCommitmentARB(GL_TEXTURE_2D_ARRAY, level, 0, 0, slice, levelWidth, levelHeight, 1, commit));
				
				levelWidth = std::max(levelWidth / 2, 1);
				levelHeight = std::max(levelHeight / 2, 1);
			}
		}
	}

	namespace texsys
	{
		void MakeNewGLTexture(r2::draw::tex::TextureHandle& handle, const r2::draw::tex::TextureFormat& format)
		{
			if (s_glTextureSystem == nullptr)
			{
				R2_CHECK(false, "We haven't initialized the GLTextureSystem yet!");
				return;
			}

			AllocGLTexture(handle, format);

			tex::Commit(handle);
		}

		void FreeGLTexture(r2::draw::tex::TextureHandle& handle)
		{
			if (s_glTextureSystem == nullptr)
			{
				R2_CHECK(false, "We haven't initialized the GLTextureSystem yet!");
				return;
			}

			tex::Free(handle);

			texcontainer::VirtualFree(*handle.container, handle.sliceIndex);
		}

		//private
		void AllocGLTexture(r2::draw::tex::TextureHandle& handle, const r2::draw::tex::TextureFormat& format)
		{
			if (s_glTextureSystem == nullptr)
			{
				R2_CHECK(false, "We haven't initialized the GLTextureSystem yet!");
				return;
			}

			r2::draw::tex::TextureContainer* containerToUse = nullptr;

			u64 intFormat = ConvertFormat(format);

			r2::SArray<r2::draw::tex::TextureContainer*>* theDefault = nullptr;

			r2::SArray<r2::draw::tex::TextureContainer*>* arrayToUse = r2::shashmap::Get(*s_glTextureSystem->texArray2Ds, intFormat, theDefault);

			if (arrayToUse == theDefault)
			{
				arrayToUse = MAKE_SARRAY(*s_glTextureSystem->arena, r2::draw::tex::TextureContainer*, s_glTextureSystem->numTextureContainersPerFormat);
				r2::shashmap::Set(*s_glTextureSystem->texArray2Ds, intFormat, arrayToUse);
			}

			const u64 arraySize = r2::sarr::Size(*arrayToUse);

			for (u64 i = 0; i < arraySize; ++i)
			{
				r2::draw::tex::TextureContainer* container = r2::sarr::At(*arrayToUse, i);
				if (container != nullptr && texcontainer::HasRoom(*container))
				{
					containerToUse = container;
					break;
				}
			}

			if (containerToUse == nullptr)
			{
				containerToUse = texcontainer::MakeGLTextureContainer<r2::mem::LinearArena>(*s_glTextureSystem->arena, s_glTextureSystem->maxNumTextureContainerLayers, format, s_glTextureSystem->sparse);
				r2::sarr::Push(*arrayToUse, containerToUse);
			}

			handle.sliceIndex = (f32)texcontainer::VirtualAlloc(*containerToUse);
			handle.container = containerToUse;
		}
	}
}


namespace r2::draw::tex::impl
{
	//Typical system stuff
	bool Init(const r2::mem::utils::MemBoundary& boundary, u32 numTextureContainers, u32 numTextureContainersPerFormat, bool useMaxNumLayers, s32 numTextureLayers, bool sparse)
	{
		if (s_glTextureSystem)
		{
			return true;
		}

		u64 numTextureContainerLayers = 0;

		if (useMaxNumLayers)
		{
			numTextureContainerLayers = GetMaxTextureLayers(true);
		}
		else
		{
			numTextureContainerLayers = numTextureLayers;
		}

		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		u64 memorySize = MemorySize(numTextureContainers, numTextureContainersPerFormat, (u32)numTextureContainerLayers, static_cast<u64>(boundary.alignment), r2::mem::LinearAllocator::HeaderSize(), boundsChecking);

		if (memorySize > boundary.size)
		{
			R2_CHECK(false, "We don't have enough memory for GLTextureSystem! Passed in: %llu, but need: %llu", boundary.size, memorySize);
			return false;
		}

		r2::mem::LinearArena* linearArena = EMPLACE_LINEAR_ARENA_IN_BOUNDARY(boundary);

		R2_CHECK(linearArena != nullptr, "We couldn't emplace the arena into the boundary!");

		s_glTextureSystem = ALLOC(GLTextureSystem, *linearArena);

		if (s_glTextureSystem == nullptr)
		{
			R2_CHECK(false, "Failed to allocate the linear arena of the GLTextureSystem");
			return false;
		}

		s_glTextureSystem->arena = linearArena;
		u32 hashMapSize = static_cast<u32>((f64)numTextureContainers * LOAD_FACTOR_MULT);
		s_glTextureSystem->texArray2Ds = MAKE_SHASHMAP(*linearArena, r2::SArray<r2::draw::tex::TextureContainer*>*, hashMapSize);

		if (s_glTextureSystem->texArray2Ds == nullptr)
		{
			R2_CHECK(false, "We couldn't allocate the texArray2Ds");
			return false;
		}

		s_glTextureSystem->boundary = boundary;
		s_glTextureSystem->numTextureContainersPerFormat = numTextureContainersPerFormat;
		s_glTextureSystem->maxNumTextureContainerLayers = static_cast<u32>(numTextureContainerLayers);
		s_glTextureSystem->sparse = sparse;

		return true;
	}

	void Shutdown()
	{
		if (!s_glTextureSystem)
			return;

		r2::mem::LinearArena* arena = s_glTextureSystem->arena;

		auto hashIter = r2::shashmap::Begin(*s_glTextureSystem->texArray2Ds);

		for (; hashIter != r2::shashmap::End(*s_glTextureSystem->texArray2Ds); ++hashIter)
		{
			r2::SArray<r2::draw::tex::TextureContainer*>* array = hashIter->value;
			if (array != nullptr)
			{
				const u64 arraySize = r2::sarr::Size(*array);

				for (u64 i = 0; i < arraySize; ++i)
				{
					r2::draw::gl::texcontainer::Destroy(*arena, r2::sarr::At(*array, i));
				}

				FREE(array, *arena);
			}
		}
		FREE(s_glTextureSystem->texArray2Ds, *arena);

		FREE(s_glTextureSystem, *arena);
		s_glTextureSystem = nullptr;

		FREE_EMPLACED_ARENA(arena);
	}

	u64 MemorySize(u32 maxNumTextureContainers, u32 maxTextureContainersPerFormat, u32 maxTextureLayers, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		u64 maxContainersInHash = static_cast<u64>((f64)maxNumTextureContainers * LOAD_FACTOR_MULT);

		return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(GLTextureSystem), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearAllocator), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::SArray<r2::draw::tex::TextureContainer*>*>::MemorySize(maxContainersInHash), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::tex::TextureContainer*>::MemorySize(maxTextureContainersPerFormat), alignment, headerSize, boundsChecking) * maxNumTextureContainers +
			r2::draw::gl::texcontainer::MemorySize(maxTextureLayers, alignment, headerSize, boundsChecking) * maxNumTextureContainers * maxTextureContainersPerFormat;
	}

	u64 GetMaxTextureLayers(bool sparse)
	{
		GLint maxTextureArrayLevels = 0;
		if (sparse) {
			GLCall(glGetIntegerv(GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS_ARB, &maxTextureArrayLevels));
		}
		else {
			GLCall(glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxTextureArrayLevels));
		}
		return maxTextureArrayLevels;
	}
}