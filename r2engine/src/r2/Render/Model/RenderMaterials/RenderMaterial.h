#ifndef __RENDER_MATERIAL_H__
#define __RENDER_MATERIAL_H__

#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Utils/Utils.h"
#include <glm/glm.hpp>

namespace r2::draw
{
	struct RenderMaterialParam
	{
		tex::TextureAddress texture;
		glm::vec4 color = glm::vec4(0); //for normal floats, we'll fill all the values with that value so when we access the channel, we get it the right value
	};

	struct RenderMaterialParams
	{
		RenderMaterialParam albedo;
		RenderMaterialParam normalMap;
		RenderMaterialParam emission;
		RenderMaterialParam metallic;

		RenderMaterialParam roughness;
		RenderMaterialParam ao;
		RenderMaterialParam height;
		RenderMaterialParam anisotropy;

		RenderMaterialParam detail;
		RenderMaterialParam clearCoat;
		RenderMaterialParam clearCoatRoughness;
		RenderMaterialParam clearCoatNormal;

		RenderMaterialParam cubemap;

		b32 doubleSided = false;
		f32 heightScale = 0.0f;
		f32 reflectance = 0.0f;
		f32 emissionStrength = 0;

		f32 alphaCutoff = 0.0f;
		f32 unused1;
		f32 unused2;
		f32 unused3;
	};
	
}

#endif // __RENDER_MATERIAL_H__
