#include "r2pch.h"
#include "r2/Render/Model/Textures/Texture.h"

namespace r2::draw::tex
{
	const char* TextureTypeToString(TextureType type)
	{
		switch (type)
		{
		case TextureType::Diffuse:
			return "texture_diffuse";
		case TextureType::Specular:
			return "texture_specular";
		case TextureType::Ambient:
			return "texture_ambient";
		case TextureType::Normal:
			return "texture_normal";
		case TextureType::Emissive:
			return "texture_emissive";
		case TextureType::Height:
			return "texture_height";
		case TextureType::Metallic:
			return "texture_metallic";
		case TextureType::MicroFacet:
			return "texture_microfacet";
		case TextureType::Occlusion:
			return "texture_occlusion";
		default:
			R2_CHECK(false, "Unsupported texture type!");
			return "";
		}
	}
}
