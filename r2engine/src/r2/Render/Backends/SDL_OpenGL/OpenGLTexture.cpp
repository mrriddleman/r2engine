#include "r2pch.h"

#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLTextureSystem.h"
#include "stb_image.h"
#include "glad/glad.h"
#include "SDL_image.h"


namespace
{
	//from stb image
	void VerticalFlip(void* image, int w, int h, int bytes_per_pixel)
	{
		int row;
		size_t bytes_per_row = (size_t)w * bytes_per_pixel;
		stbi_uc temp[2048];
		stbi_uc* bytes = (stbi_uc*)image;

		for (row = 0; row < (h >> 1); row++) {
			stbi_uc* row0 = bytes + row * bytes_per_row;
			stbi_uc* row1 = bytes + (h - row - 1) * bytes_per_row;
			// swap row0 with row1
			size_t bytes_left = bytes_per_row;
			while (bytes_left) {
				size_t bytes_copy = (bytes_left < sizeof(temp)) ? bytes_left : sizeof(temp);
				memcpy(temp, row0, bytes_copy);
				memcpy(row0, row1, bytes_copy);
				memcpy(row1, temp, bytes_copy);
				row0 += bytes_copy;
				row1 += bytes_copy;
				bytes_left -= bytes_copy;
			}
		}
	}
}


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
		//@TODO(Serge): we have the asset type now from the record


		int texWidth;
		int texHeight;
		int channels;
		u8* imageData = stbi_load_from_memory(
			assetCacheRecord.buffer->Data(),
			static_cast<int>(assetCacheRecord.buffer->Size()), &texWidth, &texHeight, &channels, 0);
		bool usedSTBI = true;

		SDL_Surface* imageSurface = nullptr;

		if (!imageData)
		{
			usedSTBI = false;
			SDL_RWops* ops = SDL_RWFromConstMem(assetCacheRecord.buffer->Data(), assetCacheRecord.buffer->Size());
			
			R2_CHECK(ops != nullptr, "We should be able to get the RWOps from memory");
			
			imageSurface = IMG_LoadTIF_RW(ops);

			if (imageSurface == nullptr)
			{
				R2_CHECK(false, "We failed to load the image from memory");
				assetCache->ReturnAssetBuffer(assetCacheRecord);
				return r2::draw::tex::GPUHandle{};
			}

			

			imageData = (u8*)imageSurface->pixels;

			texWidth = imageSurface->w;
			texHeight = imageSurface->h;

			channels = imageSurface->format->BytesPerPixel;

			VerticalFlip(imageData, texWidth, texHeight, channels);
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

		if (usedSTBI)
		{
			stbi_image_free(imageData);
		}
		else
		{
			SDL_FreeSurface(imageSurface);
		}

		assetCache->ReturnAssetBuffer(assetCacheRecord);

		return newHandle;
	}

	TextureHandle UploadToGPU(const CubemapTexture& cubemap)
	{
		stbi_set_flip_vertically_on_load(false);

		r2::asset::AssetCache* assetCache = r2::asset::lib::GetAssetCache(cubemap.sides[0].textureAssetHandle.assetCache);

		if (!assetCache)
		{
			return r2::draw::tex::GPUHandle{};
		}

		r2::draw::tex::GPUHandle newHandle;
		r2::draw::tex::TextureFormat textureFormat;
		GLenum format;
		GLenum internalFormat;

		for (u32 i = 0; i < r2::draw::tex::NUM_SIDES; ++i)
		{
			r2::asset::AssetCacheRecord assetCacheRecord = assetCache->GetAssetBuffer(cubemap.sides[CubemapSide::RIGHT + i].textureAssetHandle);
		
			R2_CHECK(assetCacheRecord.type == r2::asset::CUBEMAP_TEXTURE, "This better be a cubemap!");

			int texWidth;
			int texHeight;
			int channels;
			u8* imageData = stbi_load_from_memory(
				assetCacheRecord.buffer->Data(),
				static_cast<int>(assetCacheRecord.buffer->Size()), &texWidth, &texHeight, &channels, 0);

			if (i == 0)
			{
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

				textureFormat.internalformat = internalFormat;
				textureFormat.width = texWidth;
				textureFormat.height = texHeight;
				textureFormat.mipLevels = 1;
				textureFormat.isCubemap = true;

				r2::draw::gl::texsys::MakeNewGLTexture(newHandle, textureFormat);
			}
			
			r2::draw::gl::tex::TexSubCubemapImage2D(newHandle, static_cast<CubemapSide>(CubemapSide::RIGHT + i), 0, 0, 0, texWidth, texHeight, format, GL_UNSIGNED_BYTE, imageData);


			stbi_image_free(imageData);
			assetCache->ReturnAssetBuffer(assetCacheRecord);
		}


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