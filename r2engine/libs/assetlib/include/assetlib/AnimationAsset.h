#ifndef __ASSET_LIB_ANIMATION_ASSET_H__
#define __ASSET_LIB_ANIMATION_ASSET_H__

#include <cstdint>

namespace flat
{
	struct RAnimationMetaData;
	struct RAnimation;
}

namespace r2::assets::assetlib
{
	struct AssetFile;

	const flat::RAnimationMetaData* read_animation_meta_data(const AssetFile& file);

	const flat::RAnimation* read_animation_data(const flat::RAnimationMetaData* metaData, const AssetFile& file);

	void pack_animation(AssetFile& file, uint8_t* metaBuffer, size_t metaBufferSize, uint8_t* dataBuffer, size_t dataBufferSize);
}

#endif