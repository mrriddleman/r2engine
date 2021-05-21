#ifndef __TEXTURE_PACK_H__
#define __TEXTURE_PACK_H__

#include "r2/Core/Assets/Asset.h"
#include "r2/Render/Model/Textures/Texture.h"

#define MAKE_TEXTURE_PACK(arena, capcity) r2::draw::tex::MakeTexturePack(arena, capcity, __FILE__, __LINE__, "")
#define FREE_TEXTURE_PACK(arena, texturePackPtr) r2::draw::tex::FreeTexturePack(arena, texturePackPtr, __FILE__, __LINE__, "")

namespace r2::draw::tex
{

	struct CubemapSideEntry
	{
		r2::asset::Asset asset;
		CubemapSide side;
	};

	struct MipLevelMetaData
	{
		u32 mipLevel;
		CubemapSideEntry sides[NUM_SIDES];
	};

	struct TexturePackMetaData
	{
		MipLevelMetaData mipLevelMetaData[MAX_MIP_LEVELS];
		u32 numLevels;
		r2::asset::AssetType assetType;
	};

	struct TexturePack
	{
		u64 packName = 0;
		u64 totalNumTextures = 0;
		TexturePackMetaData metaData;
		r2::SArray<r2::asset::Asset>* albedos = nullptr;
		r2::SArray<r2::asset::Asset>* normals = nullptr;
		r2::SArray<r2::asset::Asset>* speculars = nullptr;
		r2::SArray<r2::asset::Asset>* emissives = nullptr;
		r2::SArray<r2::asset::Asset>* metallics = nullptr;
		r2::SArray<r2::asset::Asset>* occlusions = nullptr;
		r2::SArray<r2::asset::Asset>* micros = nullptr;
		r2::SArray<r2::asset::Asset>* heights = nullptr;
	};


	//@TODO(Serge): make this so that we don't allocate stuff we don't need - right now we allocate for all texture types which is unneeded
	u64 TexturePackMemorySize(u64 capacity, u64 alignment, u32 headerSize, u32 boundsChecking);

	template<class ARENA>
	TexturePack* MakeTexturePack(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description)
	{
		R2_CHECK(capacity > 0, "You're trying to allocate a texture pack with 0 capacity!");
		if (capacity == 0)
			return nullptr;

		TexturePack* newTexturePack = ALLOC_VERBOSE(TexturePack, arena, file, line, description);
		if (!newTexturePack)
		{
			R2_CHECK(false, "We couldn't allocate a new TexturePack!");
			return nullptr;
		}

		newTexturePack->albedos = MAKE_SARRAY_VERBOSE(arena, r2::asset::Asset, capacity, file, line, description);
		newTexturePack->normals = MAKE_SARRAY_VERBOSE(arena, r2::asset::Asset, capacity, file, line, description);
		newTexturePack->speculars = MAKE_SARRAY_VERBOSE(arena, r2::asset::Asset, capacity, file, line, description);
		newTexturePack->emissives = MAKE_SARRAY_VERBOSE(arena, r2::asset::Asset, capacity, file, line, description);
		newTexturePack->metallics = MAKE_SARRAY_VERBOSE(arena, r2::asset::Asset, capacity, file, line, description);
		newTexturePack->occlusions = MAKE_SARRAY_VERBOSE(arena, r2::asset::Asset, capacity, file, line, description);
		newTexturePack->micros = MAKE_SARRAY_VERBOSE(arena, r2::asset::Asset, capacity, file, line, description);
		newTexturePack->heights = MAKE_SARRAY_VERBOSE(arena, r2::asset::Asset, capacity, file, line, description);

		return newTexturePack;
	}

	template<class ARENA>
	void FreeTexturePack(ARENA& arena, TexturePack* pack, const char* file, s32 line, const char* description)
	{
		if (pack == nullptr)
		{
			R2_CHECK(false, "You're trying to free a null texture pack!");
			return;
		}
		//@NOTE: reverse order
		FREE(pack->heights, arena);
		FREE(pack->micros, arena);
		FREE(pack->occlusions, arena);
		FREE(pack->metallics, arena);
		FREE(pack->emissives, arena);
		FREE(pack->speculars, arena);
		FREE(pack->normals, arena);
		FREE(pack->albedos, arena);

		FREE_VERBOSE(pack, arena, file, line, description);
	}
}

#endif // __TEXTURE_PACK_H__
