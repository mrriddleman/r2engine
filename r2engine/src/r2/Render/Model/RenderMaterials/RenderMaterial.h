#ifndef __RENDER_MATERIAL_H__
#define __RENDER_MATERIAL_H__

#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Utils/Utils.h"
#include <glm/glm.hpp>

namespace r2::draw
{
	struct GPUMaterialTexture
	{
		tex::TextureAddress texture;
		glm::vec4 color = glm::vec4(0); //for normal floats, we'll fill all the values with that value so when we access the channel, we get it the right value
	};

	struct GPURenderTextures
	{
		GPUMaterialTexture albedo;
		GPUMaterialTexture normalMap;
		GPUMaterialTexture emission;
		GPUMaterialTexture metallic;
		GPUMaterialTexture roughness;
		GPUMaterialTexture ao;
		GPUMaterialTexture height;
		GPUMaterialTexture anisotropy;
		GPUMaterialTexture detail;
		GPUMaterialTexture clearCoat;
		GPUMaterialTexture clearCoatRoughness;
		GPUMaterialTexture clearCoatNormal;
	};

	union GPURenderTextureUnion
	{
		GPURenderTextures renderTextures;
		GPUMaterialTexture renderTextureArray[tex::Cubemap] = { {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {} };
	};

	struct GPURenderMaterial
	{
		GPURenderTextureUnion gpuTextures;

		b32 doubleSided = false;
		f32 heightScale = 0.0f;
		f32 reflectance = 0.0f;
		s32 padding = 0;
	};

	
}

#endif // __RENDER_MATERIAL_H__
