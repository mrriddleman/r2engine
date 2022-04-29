#ifndef __ASSET_LIB_TEXTURE_ASSET_H__
#define __ASSET_LIB_TEXTURE_ASSET_H__

#include "assetlib/TextureMetaData_generated.h"

namespace r2::assets::assetlib
{
	struct AssetFile;

	const flat::TextureMetaData* read_texture_meta_data(const AssetFile& file);

	void unpack_texture(const flat::TextureMetaData* info, const char* sourcebuffer, size_t sourceSize, char* destination);

	void unpack_texture_page(const flat::TextureMetaData* info, int pageIndex, char* sourcebuffer, char* destination);

	void pack_texture(AssetFile& file, flat::TextureMetaData* info, void* pixelData);
}


#endif // __ASSET_LIB_TEXTURE_ASSET_H__
