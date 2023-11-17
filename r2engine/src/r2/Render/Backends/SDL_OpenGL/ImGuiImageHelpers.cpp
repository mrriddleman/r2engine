#include "r2pch.h"
#if defined( R2_EDITOR) && defined(R2_IMGUI)
#include "r2/Render/Backends/SDL_OpenGL/ImGuiImageHelpers.h"
#include <glad/glad.h>
#include "stb_image.h"

namespace r2::edit
{
	bool GetImageFormat(int numChannels, unsigned int& internalFormat, unsigned int& imageFormat)
	{
		bool success = false;

		if (numChannels == 3)
		{
			imageFormat = GL_RGB;
			internalFormat = GL_RGB8;

			success = true;
		}
		else if (numChannels == 1)
		{
			imageFormat = GL_RED;
			internalFormat = GL_R8;

			success = true;
		}
		else if (numChannels == 4)
		{
			imageFormat = GL_RGBA;
			internalFormat = GL_RGBA8;

			success = true;
		}

		return success;
	}


	unsigned int CreateTextureFromFile(const std::string& imagePath, int& width, int& height, unsigned int wrapMode, unsigned int magFilter, unsigned int minFilter)
	{
		GLuint texture;
		//Generate a new texture
		glGenTextures(1, &texture);
		//bind it so that we tell OpenGL which texture we're working on. Out binding point here is GL_TEXTURE_2D
		glBindTexture(GL_TEXTURE_2D, texture);
		//Make our texture wrap if we sample outside of the 0.0 - 1.0 range of the UVs
		//We can see this work if you set the u_uvScale to be != to 1
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
		// set texture filtering parameters

		if (minFilter == 0)
		{
			//By default filter our textures bilinearly using GL_LINEAR unless otherwise specified
			minFilter = GL_LINEAR;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

		int nrChannels;
		stbi_set_flip_vertically_on_load(true);
		//Load the image using stbi_load
		unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			//glTexImage2D actually creates the texture data on the GPU and uploads the data to our texture
			//Since GL_TEXTURE_2D is bound to texture, that's whats being updated

			//We need to distinguish between the different types of texture based on the number of channels
			//the texture has. stb_image gives us the number of channels in the stbi_load function
			if (nrChannels == 3)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			}
			else if (nrChannels == 1)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
			}
			else if (nrChannels == 4)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			}

			//If we're using a min filter mip map - then generate the mips
			if (minFilter == GL_LINEAR_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_NEAREST ||
				minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_NEAREST_MIPMAP_NEAREST)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
			}

			//free the data since it's already on the GPU now
			stbi_image_free(data);
		}
		else
		{
			printf("Failed to load texture: %s\n", imagePath.c_str());
			assert(false);
		}

		return texture;
	}
}

#endif