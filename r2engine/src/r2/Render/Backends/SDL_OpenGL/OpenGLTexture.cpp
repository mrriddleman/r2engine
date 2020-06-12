#include "r2pch.h"

#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "stb_image.h"
#include "glad/glad.h"

namespace r2::draw::tex
{
	
	u32 UploadToGPU(const r2::asset::AssetHandle& texture, bool generateMipMap)
	{
		stbi_set_flip_vertically_on_load(true);

		r2::asset::AssetCache* assetCache = r2::asset::lib::GetAssetCache(texture.assetCache);

		if (assetCache)
		{
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
				return 0;
			}

			GLenum format;
			GLenum internalFormat;
			if (channels == 1)
			{
				format = GL_RED;
				internalFormat = GL_RED;
			}
			else if (channels == 3)
			{
				internalFormat = GL_RGB;
				format = GL_RGB;
			}
			else if (channels == 4)
			{
				internalFormat = GL_RGBA;
				format = GL_RGBA;
			}
			else
			{
				R2_CHECK(false, "UNKNOWN image format");
			}

			u32 newTex;
			glGenTextures(1, &newTex);
			glBindTexture(GL_TEXTURE_2D, newTex);

			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, texWidth, texHeight, 0, format, GL_UNSIGNED_BYTE, imageData);
			//set the texture wrapping/filtering options
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			
			if (generateMipMap)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
			}

			stbi_image_free(imageData);

			assetCache->ReturnAssetBuffer(assetCacheRecord);

			return newTex;
		}

		return 0;
	}

	void UnloadFromGPU(const u32 texture)
	{
		glDeleteTextures(1, &texture);
	}
}

#endif