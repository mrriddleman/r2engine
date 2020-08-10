#include "r2pch.h"

#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLTextureSystem.h"
#include "stb_image.h"
#include "glad/glad.h"

namespace r2::draw::tex
{
	
	TextureHandle UploadToGPU(const r2::asset::AssetHandle& texture, bool generateMipMap)
	{
		stbi_set_flip_vertically_on_load(true);

		r2::asset::AssetCache* assetCache = r2::asset::lib::GetAssetCache(texture.assetCache);

		if (!assetCache)
		{
			return r2::draw::tex::GPUHandle{};
		}

		r2::asset::AssetCacheRecord assetCacheRecord = assetCache->GetAssetBuffer(texture);

		int texWidth;
		int texHeight;
		int channels;
		u8* imageData = stbi_load_from_memory(
			assetCacheRecord.buffer->Data(),
			static_cast<int>(assetCacheRecord.buffer->Size()), &texWidth, &texHeight, &channels, 0);

		if (!imageData)
		{
			R2_CHECK(false, "We failed to load the image from memory");
			assetCache->ReturnAssetBuffer(assetCacheRecord);
			return r2::draw::tex::GPUHandle{};
		}

		GLenum format;
		GLenum internalFormat;
		if (channels == 1)
		{
			format = GL_RED;
			internalFormat = GL_R8;
		}
		else if (channels == 3)
		{
			internalFormat = GL_RGB8;
			format = GL_RGB;
		}
		else if (channels == 4)
		{
			internalFormat = GL_RGBA8;
			format = GL_RGBA;
		}
		else
		{
			R2_CHECK(false, "UNKNOWN image format");
		}

		r2::draw::tex::GPUHandle newHandle;
		r2::draw::tex::TextureFormat textureFormat;

		textureFormat.internalformat = internalFormat;
		textureFormat.width = texWidth;
		textureFormat.height = texHeight;
		textureFormat.mipLevels = 1;

		r2::draw::gl::texsys::MakeNewGLTexture(newHandle, textureFormat);

		r2::draw::gl::tex::TexSubImage2D(newHandle, 0, 0, 0, texWidth, texHeight, format, GL_UNSIGNED_BYTE, imageData);

		stbi_image_free(imageData);

		assetCache->ReturnAssetBuffer(assetCacheRecord);

		return newHandle;
	}

	void UnloadFromGPU(TextureHandle& texture)
	{
		r2::draw::gl::texsys::FreeGLTexture(texture);
	}

	TextureAddress GetTextureAddress(const TextureHandle& handle)
	{
		return r2::draw::gl::tex::GetAddress(handle);
	}
}

#endif