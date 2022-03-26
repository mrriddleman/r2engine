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
		case TextureType::Normal:
			return "texture_normal";
		case TextureType::Emissive:
			return "texture_emissive";
		case TextureType::Height:
			return "texture_height";
		case TextureType::Metallic:
			return "texture_metallic";
		case TextureType::Roughness:
			return "texture_microfacet";
		case TextureType::Occlusion:
			return "texture_occlusion";
		case TextureType::Anisotropy:
			return "texture_anisotropy";
		default:
			R2_CHECK(false, "Unsupported texture type!");
			return "";
		}
	}

	bool TextureHandlesEqual(const TextureHandle& h1, const TextureHandle& h2)
	{
		return h1.container == h2.container && h2.sliceIndex == h2.sliceIndex;
	}

	bool TexturesEqual(const Texture& t1, const Texture& t2)
	{
		return t1.textureAssetHandle.assetCache == t2.textureAssetHandle.assetCache && t1.textureAssetHandle.handle == t2.textureAssetHandle.handle && t1.type == t2.type;
	}
}
