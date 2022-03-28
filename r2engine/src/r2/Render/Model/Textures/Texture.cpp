#include "r2pch.h"
#include "r2/Render/Model/Textures/Texture.h"

namespace r2::draw::tex
{
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
