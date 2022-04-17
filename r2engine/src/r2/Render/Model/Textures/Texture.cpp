#include "r2pch.h"
#include "r2/Render/Model/Textures/Texture.h"

namespace r2::draw::tex
{
	const char* GetTextureTypeName(TextureType type)
	{
		switch (type)
		{
		case r2::draw::tex::Diffuse:
			return "Albedo";
			break;
		case r2::draw::tex::Emissive:
			return "Emissive";
			break;
		case r2::draw::tex::Normal:
			return "Normal";
			break;
		case r2::draw::tex::Metallic:
			return "Metallic";
			break;
		case r2::draw::tex::Height:
			return "Height";
			break;
		case r2::draw::tex::Roughness:
			return "Roughness";
			break;
		case r2::draw::tex::Occlusion:
			return "AO";
			break;
		case r2::draw::tex::Anisotropy:
			return "Anisotropy";
			break;
		case r2::draw::tex::Detail:
			return "Detail";
			break;
		case r2::draw::tex::ClearCoat:
			return "Clear Coat";
			break;
		case r2::draw::tex::ClearCoatRoughness:
			return "Clear Coat Roughness";
			break;
		case r2::draw::tex::ClearCoatNormal:
			return "Clear Coat Normal";
			break;
		case r2::draw::tex::Cubemap:
			return "Cubemap";
			break;
		case r2::draw::tex::NUM_TEXTURE_TYPES:
			R2_CHECK(false, "NUM_TEXTURE_TYPES isn't a texture type");
			break;
		default:
			R2_CHECK(false, "Unsupported texture type");
			break;
		}

		return "";
	}


	bool TextureHandlesEqual(const TextureHandle& h1, const TextureHandle& h2)
	{
		return h1.container == h2.container && h2.sliceIndex == h2.sliceIndex;
	}

	bool TexturesEqual(const Texture& t1, const Texture& t2)
	{
		return t1.textureAssetHandle.assetCache == t2.textureAssetHandle.assetCache && t1.textureAssetHandle.handle == t2.textureAssetHandle.handle && t1.type == t2.type;
	}

	bool TexturesEqualExcludeType(const Texture& t1, const Texture& t2)
	{
		return t1.textureAssetHandle.assetCache == t2.textureAssetHandle.assetCache && t1.textureAssetHandle.handle == t2.textureAssetHandle.handle;
	}
}
