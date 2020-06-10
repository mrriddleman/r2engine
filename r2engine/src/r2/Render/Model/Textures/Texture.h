#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "r2/Core/Assets/AssetTypes.h"

namespace r2::draw::tex
{
	enum TextureType
	{
		Diffuse = 0,
		Specular,
		Ambient,
		Normal,
		Emissive,
		Height,
		Metallic,
		MicroFacet,
		Occlusion,
		NUM_TEXTURE_TYPES
	};

	struct Texture
	{
		r2::asset::AssetHandle textureAssetHandle;
		TextureType type = TextureType::Diffuse;
	};

	using GPUHandle = u32;

	u32 UploadToGPU(const r2::asset::AssetHandle& texture, bool generateMipMap);
	void UnloadFromGPU(const u32 texture);

	const char* TextureTypeToString(TextureType type);
}


#endif // !__TEXTURE_H__
